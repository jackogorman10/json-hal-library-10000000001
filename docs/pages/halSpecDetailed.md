# JSON HAL Library Detailed Reference

See the [overview](halSpec.md) for lifecycle and runtime requirements. RdkGponManager, RdkXdslManager, and Telco Voice Manager consume their respective product schemas through this shared library; the library itself does not define a product schema.

## Configuration and Transport
`json_hal_load_config(const char *config_file, hal_config_t *config)` reads `hal_schema_path` and `server_port`, then obtains module/version from schema `definitions.moduleName.const` and `definitions.schemaVersion.const`. The configuration loader uses a 256-byte local read buffer and unbounded copies into destination fields; do not infer arbitrary configuration-size safety.

The supplied transport uses `AF_INET`, `SOCK_STREAM`, and loopback server binding. Sending serializes a JSON string and sends its bytes without a length prefix or newline delimiter. The server processes receive chunks; the client has limited full-buffer accumulation. TCP may split or coalesce data arbitrarily. This implementation does not establish robust stream framing. Neither one-send/one-receive behavior nor arbitrary-size/segmentation support is guaranteed.

The wire envelope is custom JSON HAL, not JSON-RPC 2.0: use `module`, `version`, `action`, and `reqId`, not `jsonrpc`, `method`, and `id`. Product schemas define exact payload validation.

## Client Lifecycle
| API | Inputs, output, and result |
| --- | --- |
| `int json_hal_client_init(const char *hal_conf_path)` | Configuration path; initializes shared client state. Returns `RETURN_OK` or `RETURN_ERR`. Initialize before run/request operations. |
| `int json_hal_client_run()` | Starts client socket thread. Thread-start success is not proof of an established connection. |
| `int json_hal_is_client_connected()` | Returns `TRUE`/`FALSE`, not ordinary zero-success status. It observes current connection state. |
| `int json_hal_client_terminate()` | Cleans up library state. Coordinate outstanding requests and callbacks; header alone does not guarantee race-free concurrent teardown. |

## Requests and Replies
| API | Contract |
| --- | --- |
| `json_object *json_hal_client_get_request_header(const char *action_name)` | Builds a request header for the action. Header contains module/version/action and request correlation. Inspect allocation failure and manage the resulting JSON object's lifetime. |
| `int json_hal_client_send_and_get_reply(const json_object *request, json_object **reply)` | Sends a JSON object and blocks for reply or timeout. Check library status before inspecting reply. |
| `int json_hal_client_send_and_get_reply_with_timeout(const json_object *jrequest_msg, int timeout, json_object **reply_msg)` | Explicit timeout in seconds. Does not supply hardware cancellation or transactional rollback. |
| `int json_hal_get_result_status(const json_object *json_msg, json_bool *status)` | Parses wire result into caller-owned Boolean storage; check parser status separately from returned operation success. |
| `int json_hal_get_total_param_count(const json_object *json_msg)` | Returns parameter count; it is not a zero-success status function. |

Do not assume a reply's parameter order, transaction atomicity, retry safety, or failed-output contents without a product contract. A timeout can leave the remote operation's outcome unknown.

## Client Notifications
`int json_hal_client_subscribe_event(event_callback callback, const char *event_name, const char *event_notification_type)` registers the callback for the named event and notification type.

`event_callback` is `void (*)(const char *event_msg, const int event_msg_length)`. The payload is serialized event data with an explicit length. Do not retain the pointer beyond callback execution without a copy. The library stores a shared callback pointer, not a distinct user-context callback for every subscription.

## Server Lifecycle and Dispatch
| API | Contract |
| --- | --- |
| `int json_hal_server_init(const char *hal_conf_path)` | Load server configuration and initialize the shared server. |
| `int json_hal_server_register_action_callback(const char *action_name, const action_callback callback)` | Associate an action string with application handling. Register before serving requests. Schema enum membership does not register a callback. |
| `int json_hal_server_run()` | Start server socket thread; returns `int`, not `void`. |
| `int json_hal_server_publish_event(const char *event_name, const char *event_value)` | Publish an event to subscribers using the server representation. Validate compatibility with the product schema. |
| `int json_hal_server_terminate()` | Release server resources; synchronize application callback state before shutdown. |

`action_callback` is `int (*)(const json_object *request_msg, const int params_count, json_object *reply_msg)`. Request data is borrowed; populate the supplied reply rather than replacing the reply pointer. Return `RETURN_OK`/`RETURN_ERR` as declared.

## Common Parameter Helpers
`int json_hal_get_param(json_object *jmsg, int index, eActionType action, hal_param_t *param)` writes caller-owned `hal_param_t` output. It clears output first. GET/DELETE unpack names; SET/GET-response unpack names, types, and values. The publish/default branch does not provide equivalent unpacking and may return success without useful data. Validate action and bounds before use.

`int json_hal_add_param(json_object *jreply, eActionType action, hal_param_t *param)` mutates the supplied JSON object. The expected `params` container must already exist. GET/DELETE append names; SET/GET-response append typed values. Publish handling differs and must not be assumed to match each product schema's array shape.

`hal_param_t` stores a 256-byte name and 2048-byte textual value plus a parameter-type enum. Full-buffer `strncpy` copies do not establish universal null-termination safety. String, hex-binary, and base64 values remain text. Integer/long values are converted through C numeric conversion routines, not a general range-safe schema converter.

**Boolean asymmetry:** unpacking stores Boolean values as `"0"`/`"1"`; packing expects `"true"`/`"TRUE"` or `"false"`/`"FALSE"`. Do not round-trip the textual buffer unchanged.

These helpers use `RETURN_OK`/`RETURN_ERR`; neither successful helper execution nor an enum value proves complete schema validation.

## Server Message Helpers
| API | Contract |
| --- | --- |
| `int json_hal_get_subscribe_event_request(json_object *jmsg, int index, hal_subscribe_event_request_t *param)` | Extracts subscription data into caller storage; the structure has a 512-byte name and notification-type enum. |
| `int json_hal_add_schema_response(json_object *jreply, hal_schema_response_t *param)` | Packs schema response from caller data; `filepath` has 2048 bytes. |
| `int json_hal_add_result_status(json_object *jreply, eResult_t result)` | Packs `RESULT_SUCCESS` or `RESULT_FAILURE`; this helper enum is not the complete product wire-result string set. |

`NOTIFICATION_TYPE_ON_CHANGE` is unconditional. `NOTIFICATION_TYPE_ON_CHANGE_SYNC` and `NOTIFICATION_TYPE_ON_CHANGE_SYNC_TIMEOUT` require `JSON_BLOCKING_SUBSCRIBE_EVENT`. Never assume every schema accepts every compiled notification mode.

## Validator Wrapper
| API | Preconditions and result |
| --- | --- |
| `int json_validator_init(const char *json_schema_path)` | Non-null schema path. Creates shared validator and loads root schema. Returns validator-specific status, not library `RETURN_*` macros. |
| `int json_validator_validate_request(const char *json_string)` | Call only after successful initialization. Null input returns `VALIDATOR_RETURN_INVALID_ARGUMENTS`; reported validation failure returns `VALIDATOR_RETURN_ERR`; successful validation returns `VALIDATOR_RETURN_OK`. |
| `int json_validator_terminate()` | Deletes the shared validator if present and returns `VALIDATOR_RETURN_OK`. |

The implementation parses JSON and invokes validation without an enclosing exception handler. Malformed JSON, allocation errors, or validator exceptions are not guaranteed to become error codes. Error-handler diagnostics can include input values; sanitize sensitive input/logging.

## Test-Authoring Boundaries
Before deriving tests, preflight the selected product schema, resolve internal references and malformed alternatives, establish safe framing and ownership behavior, and identify supported server callbacks. Keep schema defects, library defects, and vendor-contract failures separate. Do not fix a schema implicitly in test code or assume that a permissive schema proves implemented functionality.

## Logging and Debugging
Implementations must not log credentials, secrets, security-sensitive information, or other sensitive data. Sanitize validator diagnostics as well as application logs.

## Source References
- [Client header](https://github.com/rdkcentral/json-hal-library/blob/develop/json_hal_client.h)
- [Client implementation](https://github.com/rdkcentral/json-hal-library/blob/develop/json_hal_client.c)
- [Server header](https://github.com/rdkcentral/json-hal-library/blob/develop/json_hal_server.h)
- [Server implementation](https://github.com/rdkcentral/json-hal-library/blob/develop/json_hal_server.c)
- [Common header](https://github.com/rdkcentral/json-hal-library/blob/develop/json_hal_common.h)
- [Common implementation](https://github.com/rdkcentral/json-hal-library/blob/develop/json_hal_common.c)
- [TCP client](https://github.com/rdkcentral/json-hal-library/blob/develop/tcp_client.c)
- [TCP server](https://github.com/rdkcentral/json-hal-library/blob/develop/tcp_server.c)
- [Validator header](https://github.com/rdkcentral/json-hal-library/blob/develop/json-schema-validator-wrapper/json_schema_validator_wrapper.h)
- [Validator implementation](https://github.com/rdkcentral/json-hal-library/blob/develop/json-schema-validator-wrapper/json_schema_validator_wrapper.cpp)
