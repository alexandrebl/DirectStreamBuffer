#!/bin/bash

# Script para testar comunicação entre containers DirectStreamBuffer
# Este script demonstra como usar o dssproto entre dois containers com MACs diferentes

echo "=== DirectStreamBuffer Communication Test ==="
echo ""

# Verificar se os containers estão rodando
echo "1. Verificando status dos containers..."
docker-compose ps

echo ""
echo "2. Informações de rede dos containers:"
echo ""

# Container 1 - Sender
echo "Container 1 (directstreambuffer):"
echo "  MAC Address: 02:42:ac:11:00:02"
docker-compose exec directstreambuffer ip addr show eth0 | grep "link/ether" || echo "  Container não está rodando"

echo ""

# Container 2 - Receiver  
echo "Container 2 (directstreambuffer-echo):"
echo "  MAC Address: 02:42:ac:11:00:03"
docker-compose exec directstreambuffer-echo ip addr show eth0 | grep "link/ether" || echo "  Container não está rodando"

echo ""
echo "3. Comandos para testar comunicação:"
echo ""
echo "No Container 1 (Sender):"
echo "  docker-compose exec directstreambuffer /bin/bash"
echo "  ./dssproto eth0 02:42:ac:11:00:03"
echo ""
echo "No Container 2 (Receiver):"
echo "  docker-compose exec directstreambuffer-echo /bin/bash"
echo "  ./dssproto eth0 02:42:ac:11:00:02"
echo ""
echo "4. Comandos úteis:"
echo ""
echo "Iniciar containers:"
echo "  docker-compose up -d"
echo ""
echo "Parar containers:"
echo "  docker-compose down"
echo ""
echo "Ver logs:"
echo "  docker-compose logs directstreambuffer"
echo "  docker-compose logs directstreambuffer-echo"
echo ""
echo "Acessar shell dos containers:"
echo "  docker-compose exec directstreambuffer /bin/bash"
echo "  docker-compose exec directstreambuffer-echo /bin/bash"
echo ""
echo "=== Configuração de Rede ==="
echo "Rede: dssproto_network (172.20.0.0/16)"
echo "Gateway: 172.20.0.1"
echo "Container 1 MAC: 02:42:ac:11:00:02"
echo "Container 2 MAC: 02:42:ac:11:00:03"
echo ""
echo "Nota: Os containers usam rede bridge personalizada para permitir"
echo "      controle total dos endereços MAC e comunicação entre eles."
