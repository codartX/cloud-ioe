import logging
import msg_define as d
import utils as u
import socket

"""
    Message format:
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Ver|    Type   |    Method     |          Message ID           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |              len              |          Device id            |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |      Device id                                                |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |      Device id                |      Parameters(json)         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    
    Ver: messgae format version
    Type: message type, Request(0)/Response(1)
    Message ID: message id in specific session
    Len: message length
    Device ID: device mac address, etc  //it could only be existed in gateway<->server message define, device to device don't need.
    Method:<new_device(0)/get_resources(1)/set_resources(2)/report(3)/upgrade(4)/reload(5)/message(6)/
            device_auth(7)/set_policy(8)/get_policy(9)/unset_policy(10)/subscribe(11)/unsubscribe(12)>
    Parameters: parameters for method
    
"""

"""
new_device:

Direction: device/gw<-->cloud

If send new device message to device by request, just same function as discover device, empty parameters

Request:
parameters:
[
    [
        [
            <obj_id>, 
            <obj_name>, 
            [
                [<res_id>, <res_name>, <value>],
                [<res_id>, <res_name>, <value>],
                ...
            ]
        ],
        ...
    ],
    <timestamp>
]

Response:
parameters:
    [<retcode>,<msg_str>]

"""

"""
report:

Direction: device/gw-->cloud device<->device

Request:
parameters:
[
    [
        [
            <obj>,
            [
                [
                    <resource>,
                    <value>
                ],
                ...
            ]
        ],
        ...
    ],
    <timestamp>
]

Response:
parameters:
    [<retcode>,<msg_str>]

"""

"""
get_resources:

Direction: device/gw<--cloud

Request:
parameters:
[
    [
        <obj>, 
        [
            <resource>,
            ...
        ]
    ],
    ...
]
or
NULL(means all info),


Response:
parameters:
[
    [
        [
            <obj>,
            [
                [
                    <resource>,
                    <value>
                ],
                ...
            ]
        ],
        ...
    ],
    <timestamp>
]
"""

"""
set resources:

Direction: cloud-->device/gw

Request:
parameters:
[
    [
        <obj>, 
        [
            [
                <resource>, 
                <value>
            ],
            ...
        ]
    ],
    ...
]

Response:
parameters:
    [<retcode>,<msg_str>,<timestamp>]


"""

"""
upgrade:

Direction: cloud-->device/gw

Request:
parameters:

Response:
parameters:
"""

"""
reload:

Direction: cloud-->device/gw

Request:
parameters:
NULL

Response:
    [<retcode>,<msg_str>]
"""

"""
message:

Direction: device/gw-->cloud

Request:
parameters:
    [<msg_level>, <msg_str>, <timestamp>]

Response:
parameters:
    [<retcode>,<msg_str>]
"""

"""
device_auth:

Direction: gw<-->cloud

Request:
parameters:
[
    <encrypted or not>, //0=plaintext, 1=encrypted
    <password>//encrypted by key
]

Response:
parameters:
    [<retcode>, <password>//plaintext]
"""

"""
set policy:
    
Direction: cloud-->device/gw
    
Request:
parameters:
[
    #policy
    [
        <id>,
        [
            #conditions
            [
                [0(type 0), <device id>, <obj>,<res>,<op>,<value>],
                [1(type 1), <exp_time>],
                [2(type 2), <interval>]
            ]
            ...
        ],
        [
            #actions
            [
                [0(type 0), <obj>, <res>, <op>, <value>],
                [1(type 1), <level>, <msg>]
            ],
            ...
        ]
    ],
    ...
]

Response:
parameters:
    [<retcode>,<msg_str>]

"""

"""
get policy:
    
Direction: device/gw<-->cloud
    
Request:
parameters:
    [<id>, ...](optional)
    
Response:
parameters:
    [policy list]
"""

"""
unset policy:
    
Direction: cloud-->device/gw
    
Request:
parameters:
    [<id>, ...]
    
Response:
parameters:
    [<retcode>,<msg_str>]
"""

"""
subscribe:
    
Direction: cloud/device-->device
    
Request:
parameters:
[
    [
        <obj>,
        [
            [
                <res>,
                [2(type 2), <op>, <value>]/[1(type 1), <exp_time>]/[0(type 0), <interval>]/[3(type 3), <value>]
            ],
            ...
        ]
    ],
    ...
]

Response:
parameters:
    [<retcode>,<msg_str>]
"""

"""
unsubscribe:
    
Direction: cloud/device-->device
    
Request:
parameters:
[
    [
        <obj>,
        [
            <res>,
            ...
        ]
    ],
    ...
]
    
Response:
parameters:
[<retcode>,<msg_str>]
"""

def build_message(msgtype, message_id, device_id, method, parameters = []):
    assert msgtype in d.TYPE_ALL
    assert method in d.METHOD_ALL
    
    message   = []
    
    # header
    message += [chr(msgtype << 2 | d.VERSION & 0x03)]
    message += [chr(method)]
    message_id = socket.htons(message_id)
    message += [chr(message_id & 0xFF), chr((message_id>>8) & 0xFF)] 
    length = len(parameters)
    message += [chr(length & 0xFF), chr((length>>8) & 0xFF)]
    message += device_id
    if parameters:
        message += parameters
    
    return ''.join(message)

def parse_message(message):
    
    returnVal = {}
    
    # header
    if len(message) < 14:
        raise ValueError(('message to short, {0} bytes: not space for header'.format(len(message))))
    
    header_string = message[:14]
    header = map(ord, header_string)
    returnVal['version'] = (header[0]>>6)&0x03
    if returnVal['version'] != d.VERSION:
        raise e.messageFormatError('invalid version {0}'.format(returnVal['version']))
    
    returnVal['msg_type'] = header[0]&0x3f
    if returnVal['msg_type'] not in d.TYPE_ALL:
        raise e.messageFormatError('invalid message type {0}'.format(returnVal['type']))
    
    returnVal['method'] = header[1]

    returnVal['message_id'] = socket.ntohs(u.buf2int(header[2:4]))

    returnVal['len'] = socket.ntohs(u.buf2int(header[4:6]))

    returnVal['device_id'] = message[6:14]

    if len(message) > 14:
        returnVal['parameters'] = message[14:14 + returnVal['len']]
    else:
        returnVal['parameters'] = []
    
    logging.debug('parsed message: {0}'.format(returnVal))
    
    return returnVal
