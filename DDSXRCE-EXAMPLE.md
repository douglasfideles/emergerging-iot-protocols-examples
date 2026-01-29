# 🛰️ Micro XRCE-DDS com Docker Compose (HelloWorld)

Este repositório demonstra como subir rapidamente um ambiente **Micro XRCE-DDS** utilizando **Docker Compose**, com:

- 🧠 **XRCE Agent**
- 📡 **Publisher (sensor)**
- 📥 **Subscriber**

Todos baseados nos exemplos oficiais da **eProsima**.

---

## 📦 Pré-requisitos

- Docker
- Docker Compose
- Sistema Linux (recomendado para `network_mode: host`)
- Permissão de `sudo` (opcional, para análise de tráfego com `tcpdump`)

---

## 📁 Estrutura do Projeto

```
.
├── docker-compose-xrce.yml
└── README.md
```

---

## 1️⃣ Criar o arquivo `docker-compose-xrce.yml`

Crie o arquivo **`docker-compose-xrce.yml`** na raiz do projeto e copie **exatamente** o conteúdo abaixo:

```yaml
version: '3.8'

services:
  xrce_agent:
    image: eprosima/micro-xrce-dds-agent:latest
    container_name: xrce_agent
    ports:
      - "8888:8888/udp"
    command: MicroXRCEAgent udp4 -p 8888
    restart: unless-stopped

  xrce_publisher:
    image: microros/micro-ros-agent:latest
    container_name: xrce_sensor_pub
    depends_on:
      - xrce_agent
    network_mode: host
    command: helloworld_pub
    restart: on-failure

  xrce_subscriber: 
    image: microros/micro-ros-agent:latest  # Mesma imagem [web:5]
    container_name: xrce_sensor_sub
    depends_on:
      - xrce_agent
    network_mode: host
    command: helloworld_sub
    restart: on-failure

```

---

## 2️⃣ Subir o ambiente

Execute o comando abaixo no diretório do projeto:

```bash
docker-compose -f docker-compose-xrce.yml up -d
```

---

## 3️⃣ Fluxo de funcionamento

1. 🧠 **XRCE Agent**
   - Inicializa primeiro
   - Escuta na porta **UDP 8888**

2. 📡 **Publisher (`helloworld_pub`)**
   - Cria entidades XRCE
   - Publica mensagens `HelloWorld`
   - Envia os dados para o Agent

3. 📥 **Subscriber (`helloworld_sub`)**
   - Conecta ao Agent
   - Recebe e imprime as mensagens

### 🔁 Fluxo lógico

```
Publisher ───► XRCE Agent ───► Subscriber
```

---

## 4️⃣ Verificar se está funcionando

### Logs do Agent
```bash
docker logs xrce_agent
```

### Logs do Publisher (sensor)
```bash
docker logs xrce_sensor_pub
```

### Logs do Subscriber
```bash
docker logs xrce_sensor_sub
```

### 📤 Saída esperada no Subscriber

```text
Message: HelloWorld
Message: HelloWorld
Message: HelloWorld
...
```

---

## 5️⃣ Análise de tráfego (estudo de segurança)

Para capturar o tráfego XRCE entre **Client ↔ Agent**:

```bash
sudo tcpdump -i any udp port 8888 -vv
```

Você deverá observar pacotes XRCE, como:

- `CREATE_ENTITY`
- `DATA`
- `DELETE_ENTITY`

Esse passo é útil para:

- 🔐 Análise de segurança
- 📊 Geração de datasets
- 🔎 Inspeção de protocolos IoT
- 🧪 Testes de IDS/IPS

---

## 6️⃣ Parar e limpar o ambiente

Para parar os containers:

```bash
docker-compose -f docker-compose-xrce.yml down -v
```

Limpeza opcional do Docker:

```bash
docker system prune -f
```

---

## ⚠️ Observações importantes

- `network_mode: host` funciona melhor em **Linux**
- Em **macOS/Windows**, pode ser necessário adaptar a configuração de rede
- O exemplo **HelloWorld** é ideal para testes, aprendizado e experimentação

