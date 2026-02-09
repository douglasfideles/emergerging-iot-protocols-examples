#!/usr/bin/env python3
"""
Simple CoAP Server for testing attacks
"""
import asyncio
import logging
from aiocoap import resource, Context, Message
from aiocoap.numbers.codes import Code

logging.basicConfig(level=logging.INFO)
logging.getLogger("coap-server").setLevel(logging.DEBUG)

class TimeResource(resource.Resource):
    async def render_get(self, request):
        import time
        payload = f"Current time: {time.ctime()}".encode('utf8')
        return Message(payload=payload)

class SensorResource(resource.Resource):
    async def render_get(self, request):
        payload = b'{"temperature":25.3,"humidity":60}'
        return Message(payload=payload, content_format=50)  # application/json

class ObservableResource(resource.ObservableResource):
    def __init__(self):
        super().__init__()
        self.counter = 0
        
    async def render_get(self, request):
        self.counter += 1
        payload = f"Counter: {self.counter}".encode('utf8')
        return Message(payload=payload)
        
    async def update_state(self):
        """Periodically update and notify observers"""
        while True:
            await asyncio.sleep(5)
            self.updated_state()

async def main():
    # Create resource tree
    root = resource.Site()
    root.add_resource(['time'], TimeResource())
    root.add_resource(['sensor', 'temp'], SensorResource())
    root.add_resource(['observable'], ObservableResource())
    root.add_resource(['test'], resource.Resource())  # Simple resource
    
    # Start server
    await Context.create_server_context(root, bind=('0.0.0.0', 5683))
    
    print("CoAP Server started on port 5683")
    print("Available resources:")
    print("  coap://localhost/time")
    print("  coap://localhost/sensor/temp")
    print("  coap://localhost/observable")
    print("  coap://localhost/test")
    print("")
    print("Server is ready for attacks!")
    
    # Keep running
    await asyncio.get_running_loop().create_future()

if __name__ == "__main__":
    asyncio.run(main())
