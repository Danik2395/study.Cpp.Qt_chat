# Qt chat lab

## Pipeline

### Server
- Server starts
- Server listen UDP requests
- Server listen to TCP (messages)
- On valid UDP request responds to requesting socket\
    - request is `SERVER_SEARCH_REQUEST` with `sender_id == SECRET_CODE`
- On TCP `JOIN_ROOM`
    - check `room_id` / `sender_id` length, on fail send `INFO` and stop
    - add socket to room. If no room creates it
    - send `ROOM_JOINED` (with `room_id` + `sender_id`) back to the socket
    - broadcast `CHAT_INFO_MSG` about join to the room
- On TCP `CHAT_USR_MSG` broadcast message to the room
- On TCP `LEAVE_ROOM`
    - remove socket from room
    - send `ROOM_LEFT` back to the socket
    - broadcast `CHAT_INFO_MSG` about leave to the room
- On socket disconnected remove it from its room

### Client
- Sends UDP discovery request (`SERVER_SEARCH_REQUEST`) to loopback and broadcast
- On valid `SERVER_SEARCH_RESPONCE` connects TCP to the responder
- On TCP `connected` notifies UI (`connected_to_server`)
- Sends `JOIN_ROOM` with room id and user name
- On `ROOM_JOINED` remembers room / name and opens chat screen
- Sends `CHAT_USR_MSG` to the room
- On leave sends `LEAVE_ROOM`, on `ROOM_LEFT` returns to login screen
- On `CHAT_USR_MSG` / `CHAT_INFO_MSG` appends message to chat
- On `INFO` shows status on login screen
- On socket disconnected clears room / name and returns to login screen

## Message
- Fields: `type`, `sender_id`, `room_id`, `payload`
- Types:\
`JOIN_ROOM`,\
`ROOM_JOINED`,\
`LEAVE_ROOM`,\
`ROOM_LEFT`,\
`CHAT_USR_MSG`,\
`CHAT_INFO_MSG`,\
`INFO`,\
`SERVER_SEARCH_REQUEST`,\
`SERVER_SEARCH_RESPONCE`
