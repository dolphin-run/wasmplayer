
import asyncio
import websockets

fin = open("wsserver/outfile.h264", 'rb')

buf = fin.read()
#strb = buf.decode('ascii')
#print(buf.index(b'\x00\x00\x00\x01'))

result = buf.split(b'\x00\x00\x00\x01')
header = r'{"action":"init","width":1920,"height":1080}'
result[0] = header#.encode('ascii')

def print_hex(bytes):
    for i in bytes:
        print('0x%02x'%(int(i)), end = ' ')
    print("\n")

async def hello(websocket, path):
    idx = 1
    while True:
        name = await websocket.recv()
        #print(f"< {name}")

        if idx == len(result): 
            #idx = 1
            break

        if idx > 0:
            greeting = b'\x00\x00\x00\x01'+result[idx]
        else:
            greeting = result[idx]
        await websocket.send(greeting)
        print(f"> idx={idx}, {len(greeting)}")
        #print(print_hex(greeting))
        idx += 1

start_server = websockets.serve(hello, '127.0.0.1', 12345)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()