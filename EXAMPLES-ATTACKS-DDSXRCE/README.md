# DDS-XRCE Attack Examples

Exemplos de ataques de segurança contra o protocolo DDS-XRCE (DDS for eXtremely Resource Constrained Environments), usado em sistemas IoT.

---

## 🚀 Quick Start (3 comandos)

```bash
# 1. Instale dependências e compile
./setup_dependencies.sh && ./compile_all.sh

# 2. Rode o agent (Terminal 1)
./run_agent_docker.sh

# 3. Execute ataques (Terminal 2)
./bin/attack_session_hijack 127.0.0.1 2018
```

**Pronto!** Veja seções abaixo para detalhes.

---

## 📋 Índice

- [Instalação Detalhada](#-instalação-detalhada)
- [Descrição dos Ataques](#-descrição-dos-ataques)
- [Estrutura dos Arquivos](#-estrutura-dos-arquivos)
- [Troubleshooting](#-troubleshooting)
- [Segurança e Defesas](#-segurança-e-defesas)

---

## 🔧 Instalação Detalhada

### Requisitos Mínimos

- Linux (Ubuntu 18.04+, Debian 10+, ou similar)
- GCC 7.5+ e CMake (build-essential)
- Docker (para rodar o agent - recomendado)

### Passo 1: Instalar Dependências e Compilar

```bash
chmod +x setup_dependencies.sh compile_all.sh
./setup_dependencies.sh  # Instala Micro-CDR e Micro XRCE-DDS Client (~10MB)
./compile_all.sh         # Compila os 7 ataques
```

**Tempo:** ~5-10 minutos

**O que isso instala:**

- **Micro-CDR** - Biblioteca de serialização (necessária)
- **Micro XRCE-DDS Client** - Biblioteca DDS-XRCE (necessária)

**Executáveis criados em `./bin/`:**

- `attack_session_hijack` - Sequestro de sessão
- `attack_entity_flood` - Flood de entidades
- `attack_ping_flood` - Flood de pings
- `attack_time_desync` - Dessincronização de tempo
- `attack_malformed_inject` - Injeção de mensagens malformadas
- `attack_fragment_abuse` - Abuso de fragmentação
- `attack_discovery_poison` - Envenenamento de discovery

### Passo 2: Rodar o Agent (Escolha uma opção)

#### Opção A: Docker (⭐ Recomendado)

Mais rápido e leve - não instala dependências pesadas (~500MB)

```bash
chmod +x run_agent_docker.sh
./run_agent_docker.sh
```

#### Opção B: Binário Pré-compilado

Download direto do executável sem compilar:

```bash
chmod +x download_agent.sh
./download_agent.sh
./agent/MicroXRCEAgent udp4 -p 2018
```

#### Opção C: Agent Remoto

Se já tem um agent DDS-XRCE rodando em outro servidor, pule a instalação do agent

#### Opção D: Compilar do Zero (Não recomendado)

Instala Fast-DDS + dependências (~500MB) e compila tudo:

```bash
chmod +x setup_agent.sh
./setup_agent.sh  # Demora 10-20 minutos
MicroXRCEAgent udp4 -p 2018
```

---

## 🎯 Descrição dos Ataques

### Terminal 1: Inicie o Agent

```bash
./run_agent_docker.sh

# Output esperado:
# [** Micro XRCE-DDS Agent **]
# [INFO] UDP agent initialization...
# [INFO] Listening on port 2018
```

### Terminal 2: Execute os Ataques

#### 1. Session Hijack

Tenta sequestrar sessões usando chaves de sessão previsíveis/comuns

```bash
./bin/attack_session_hijack 127.0.0.1 2018
```

**O que observar:**

- Múltiplas tentativas de criação de sessão
- Possível aceitação de sessões com chaves fracas
- Injeção de mensagens maliciosas na sessão

---

#### 2. Entity Flood

Cria milhares de entidades DDS (participants, topics, publishers, datawriters) para esgotar recursos

```bash
./bin/attack_entity_flood 127.0.0.1 2018
```

**O que observar:**

- Criação massiva de entidades
- Aumento de uso de memória e CPU no agent
- Possível lentidão ou travamento
- **ATENÇÃO:** Alto consumo de recursos!

---

#### 3. Ping Flood

Sobrecarrega o agent com requisições de ping

```bash
./bin/attack_ping_flood 127.0.0.1 2018
```

**O que observar:**

- Alto volume de requisições de ping
- Aumento de uso de CPU e rede
- Possível degradação de desempenho

---

#### 4. Time Desync

Tenta desincronizar o relógio entre cliente e agent corrompendo timestamps

```bash
./bin/attack_time_desync 127.0.0.1 2018
```

**O que observar:**

- Múltiplas requisições de sincronização de tempo
- Timestamps potencialmente corrompidos
- Comportamento temporal anômalo

---

#### 5. Malformed Message Injection

Injeta mensagens malformadas para testar robustez do parser

```bash
./bin/attack_malformed_inject 127.0.0.1 2018
```

**O que observar:**

- Erros de parsing
- Warnings sobre dados inválidos
- **Possíveis crashes do agent**
- Buffer overflows potenciais

---

#### 6. Fragment Abuse

Abusa do sistema de fragmentação de mensagens grandes

```bash
./bin/attack_fragment_abuse 127.0.0.1 2018
```

**O que observar:**

- Fragmentos incompletos ou sobrepostos
- Buffers de reassembly cheios
- Possível denial of service
- Fragmentos órfãos não liberados

---

#### 7. Discovery Poison

Envenena o protocolo de descoberta de agents (requer privilégios root)

```bash
sudo ./bin/attack_discovery_poison 192.168.1.100
```

**Substitua** `192.168.1.100` pelo IP do seu sistema malicioso

**O que observar:**

- Clientes redirecionados para agent falso
- Respostas de discovery falsificadas
- Man-in-the-middle potential

---

### Menu Interativo (Alternativa)

Para facilitar a execução, use o script de testes:

```bash
chmod +x run_tests.sh
./run_tests.sh

# Menu disponível:
# 1. Session Hijack
# 2. Entity Flood
# 3. Ping Flood
# 4. Time Desync
# 5. Malformed Inject
# 6. Fragment Abuse
# 7. Discovery Poison
# 8. Executar TODOS
```

---

## 📁 Estrutura dos Arquivos

```
EXAMPLES-ATTACKS-DDSXRCE/
│
├── 📄 Código-fonte dos ataques
│   ├── attack_session_hijack.c
│   ├── attack_entity_flood.c
│   ├── attack_ping_flood.c
│   ├── attack_time_desync.c
│   ├── attack_malformed_inject.c
│   ├── attack_fragment_abuse.c
│   └── attack_discovery_poison.c
│
├── 🔧 Scripts de instalação
│   ├── setup_dependencies.sh    # Instala libs DDS (necessário)
│   └── compile_all.sh           # Compila todos os ataques
│
├── 🐳 Scripts do agent (escolha um)
│   ├── run_agent_docker.sh      # Agent via Docker (recomendado)
│   ├── download_agent.sh        # Download binário pré-compilado
│   └── setup_agent.sh           # Compilar do zero (~500MB)
│
├── 🧪 Scripts de teste
│   └── run_tests.sh             # Menu interativo
│
├── 📖 Documentação
│   ├── README.md                # Este arquivo
│   └── .gitignore               # Ignora bin/ e logs/
│
└── 📦 Gerado após compilação
    └── bin/                     # Executáveis compilados
        ├── attack_session_hijack
        ├── attack_entity_flood
        └── ...
```

### Detalhes dos Scripts

**setup_dependencies.sh**

- Instala Micro-CDR (serialização de dados)
- Instala Micro XRCE-DDS Client (biblioteca DDS)
- **Obrigatório** - sem isso os ataques não compilam
- Tamanho: ~10 MB instalado

**compile_all.sh**

- Compila os 7 arquivos `.c` em executáveis
- Cria a pasta `bin/` automaticamente
- Valida dependências antes de compilar

**run_agent_docker.sh**

- Roda o agent usando imagem Docker oficial
- Não requer instalação de dependências
- **Recomendado** para testes

**download_agent.sh**

- Baixa binário pré-compilado do agent
- Alternativa ao Docker
- Útil se não tiver Docker instalado

**setup_agent.sh**

- Compila agent + Fast-DDS do zero
- **Pesado**: ~500 MB de dependências
- Demora 10-20 minutos
- Use apenas se necessário

**run_tests.sh**

- Menu interativo para executar ataques
- Evita digitar comandos longos
- Valida se binários existem

---

## 📊 Monitoramento (Opcional)

### Wireshark

Para análise detalhada do tráfego DDS-XRCE:

```bash
sudo apt install wireshark
sudo wireshark -i lo -f "udp port 2018"
```

**Filtros úteis:**

- `udp.port == 2018` - Todo tráfego DDS-XRCE
- `udp.length > 1000` - Mensagens grandes (fragmentos)
- `ip.src == 127.0.0.1 && udp.port == 2018` - Tráfego dos ataques

### tcpdump

Captura de pacotes via linha de comando:

```bash
sudo tcpdump -i lo udp port 2018 -w capture.pcap
```

---

## 🛠️ Troubleshooting

### Erro: "microxrcedds_client não encontrado"

**Causa:** Bibliotecas DDS não instaladas

```bash
# Reinstale as dependências
./setup_dependencies.sh

# Verifique instalação
ls -la /usr/local/lib/libmicroxrcedds_client.*
```

---

### Erro: "Failed to init transport"

**Causa:** Agent não está rodando ou porta incorreta

```bash
# Verifique se agent está rodando
docker ps                           # Se usando Docker
ps aux | grep MicroXRCEAgent        # Se compilado

# Teste conectividade
nc -zvu 127.0.0.1 2018

# Verifique se porta está aberta
sudo netstat -tulpn | grep 2018
```

---

### Porta já em uso

```bash
# Encontre processo usando a porta
sudo netstat -tulpn | grep 2018

# Mate o processo
sudo kill -9 <PID>

# Ou use outra porta
./run_agent_docker.sh   # edite para usar -p 8888
./bin/attack_session_hijack 127.0.0.1 8888
```

---

### Erro de compilação: "header not found"

**Causa:** Caminhos de include incorretos

```bash
# Verifique se headers foram instalados
ls -la /usr/local/include/uxr/client/
ls -la /usr/local/microcdr-2.0.2/include/ucdr/

# Reinstale dependências
./setup_dependencies.sh
```

---

### Docker: "Cannot connect to daemon"

```bash
# Inicie o Docker
sudo systemctl start docker

# Ou instale Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
```

---

## ⚠️ Avisos de Segurança

### Uso Ético

Estes ataques são para **fins educacionais e de pesquisa** apenas:

✅ **PERMITIDO:**

- Ambientes controlados e autorizados
- Laboratórios de pesquisa
- Redes isoladas de teste
- Com permissão explícita por escrito

❌ **PROIBIDO:**

- Sistemas em produção sem autorização
- Redes de terceiros
- Qualquer uso ilegal ou malicioso

⚖️ **Responsabilidade Legal:**

- Respeite todas as leis locais e internacionais
- Obtenha autorização por escrito antes de testar
- Use apenas em ambientes que você controla

### Ambiente de Teste Recomendado

- 🖥️ Máquina virtual isolada (VirtualBox, VMware)
- 🔒 Rede privada sem conexão à internet
- 🐳 Containers Docker/LXC
- ☁️ Cloud sandbox (AWS, GCP, Azure com VPC isolada)

### Impacto dos Ataques

Alguns ataques podem causar:

- ⚠️ Alto uso de CPU (80-100%)
- ⚠️ Alto uso de memória (pode esgotar RAM)
- ⚠️ Crash do agent
- ⚠️ Negação de serviço temporária
- ⚠️ Preenchimento de logs (GB de dados)

**Execute com cuidado** e monitore os recursos do sistema.

---

## 🔐 Defesas Recomendadas

Com base nestes ataques, implemente as seguintes defesas:

### 1. Autenticação Forte

- Use chaves de sessão criptográficas (não previsíveis)
- Implemente mutual TLS (mTLS)
- Rotacione chaves periodicamente

### 2. Rate Limiting

- Limite requisições por cliente (ex: 100/segundo)
- Limite criação de entidades por sessão
- Timeout para handshakes incompletos

### 3. Validação de Entrada

- Rejeite mensagens malformadas imediatamente
- Valide tamanhos de campos antes de processar
- Use bounds checking rigoroso

### 4. Limites de Recursos

- Máximo de entidades por sessão (ex: 100)
- Máximo de fragmentos pendentes (ex: 50)
- Timeout para fragmentos incompletos (ex: 30s)

### 5. Monitoramento

- Detecte padrões anormais (muitas conexões, entidades)
- Alerte sobre sessões com chaves comuns
- Log de todas as operações suspeitas

### 6. Segmentação de Rede

- Isole dispositivos IoT em VLAN separada
- Use firewall para controlar tráfego DDS
- Implemente IDS/IPS

---

## 📚 Referências Técnicas

### Especificações

- [DDS-XRCE v1.0 Specification (OMG)](https://www.omg.org/spec/DDS-XRCE/)
- [DDS v1.4 Specification](https://www.omg.org/spec/DDS/)
- [RTPS v2.3 Protocol](https://www.omg.org/spec/DDSI-RTPS/)

### Documentação

- [Micro XRCE-DDS Documentation](https://micro-xrce-dds.docs.eprosima.com/)
- [eProsima GitHub](https://github.com/eProsima)
- [Micro-CDR API Reference](https://micro-xrce-dds.docs.eprosima.com/en/latest/microcdr.html)

### Pesquisa em Segurança

- [IoT Security: DDS Attack Surface Analysis](https://research.nccgroup.com/)
- [Vulnerabilities in DDS Implementations](https://www.usenix.org/publications)

---

## 💬 Suporte e Contribuições

### Reportar Problemas

- 🐛 Abra uma issue no repositório
- 📧 Descreva o erro, ambiente, e logs
- 🔍 Verifique se já não foi reportado

### Contribuir

Pull requests são bem-vindos para:

- Novos ataques
- Melhorias nos ataques existentes
- Correções de bugs
- Melhorias na documentação

---

**Desenvolvido para pesquisa em segurança de protocolos IoT**

_Última atualização: Janeiro 2026_ 3. Validação rigorosa de entrada 4. Limites de recursos por sessão 5. Timeouts para fragmentos incompletos

## 📚 Referências

- [DDS-XRCE Specification](https://www.omg.org/spec/DDS-XRCE/)
- [Micro XRCE-DDS Docs](https://micro-xrce-dds.docs.eprosima.com/)

---

**Desenvolvido para pesquisa em segurança de protocolos IoT**
