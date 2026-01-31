#!/bin/bash

echo "=== Rodando Micro XRCE-DDS Agent via Docker ==="
echo ""

# Pull da imagem oficial
docker pull eprosima/micro-xrce-dds-agent:latest

# Rodar agent na porta 2018
docker run -it --rm \
  --network host \
  eprosima/micro-xrce-dds-agent:latest \
  MicroXRCEAgent udp4 -p 2018

# Ou com porta mapeada:
# docker run -it --rm -p 2018:2018/udp \
#   eprosima/micro-xrce-dds-agent:latest \
#   MicroXRCEAgent udp4 -p 2018
