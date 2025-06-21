// directstreamservice.cpp
// Compile with: g++ -std=c++17 directstreamservice.cpp -o dssproto
// Run (as root): sudo ./dssproto <interface> <dest-mac>

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// Custom Ethertype (0x88B5 é um valor não‐atribuído, só para experimentação)
static constexpr uint16_t ETH_P_CUSTOM = 0x88B5;

// Pacote: 1 byte tipo, 2 bytes tamanho, 4 bytes seq, 2 bytes CRC16
#pragma pack(push, 1)
struct PacketHeader {
    uint8_t  type;
    uint16_t length;
    uint32_t seq;
    uint16_t crc16;
};
#pragma pack(pop)

// CRC16-CCITT simples
uint16_t crc16_ccitt(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc ^= (uint16_t)(*data++) << 8;
        for (int i = 0; i < 8; i++)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
    return crc;
}

// Converte string “AA:BB:CC:DD:EE:FF” em 6 bytes
std::array<uint8_t, 6> parse_mac(const std::string& mac) {
    std::array<uint8_t, 6> b;
    if (std::sscanf(mac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                    &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6)
        throw std::invalid_argument("Formato MAC inválido");
    return b;
}

// Converte 6 bytes em string "AA:BB:CC:DD:EE:FF"
std::string format_mac(const uint8_t* mac) {
    char buf[18];
    std::snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(buf);
}

class RawSocket {
public:
    RawSocket(const std::string& ifname) {
        // Cria socket RAW
        fd_ = ::socket(AF_PACKET, SOCK_RAW, htons(ETH_P_CUSTOM));
        if (fd_ < 0) throw std::runtime_error("socket(): " + std::string(strerror(errno)));

        // Índice da interface
        struct ifreq ifr = {};
        std::strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ - 1);
        if (ioctl(fd_, SIOCGIFINDEX, &ifr) < 0)
            throw std::runtime_error("ioctl(SIOCGIFINDEX): " + std::string(strerror(errno)));
        int ifindex = ifr.ifr_ifindex;

        // MAC de origem
        if (ioctl(fd_, SIOCGIFHWADDR, &ifr) < 0)
            throw std::runtime_error("ioctl(SIOCGIFHWADDR): " + std::string(strerror(errno)));
        for (int i = 0; i < 6; i++)
            src_mac_[i] = (uint8_t)ifr.ifr_hwaddr.sa_data[i];

        // Bind na interface
        struct sockaddr_ll addr = {};
        addr.sll_family   = AF_PACKET;
        addr.sll_protocol = htons(ETH_P_CUSTOM);
        addr.sll_ifindex  = ifindex;
        if (bind(fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0)
            throw std::runtime_error("bind(): " + std::string(strerror(errno)));
    }

    ~RawSocket() {
        if (fd_ >= 0) ::close(fd_);
    }

    const std::array<uint8_t,6>& src_mac() const { return src_mac_; }

    // Envia um frame Ethernet completo (dst_mac + src_mac + ethertype + payload)
    void send_frame(const std::array<uint8_t,6>& dst_mac,
                    const uint8_t* data, size_t len) 
    {
        std::vector<uint8_t> frame(14 + len);
        std::memcpy(frame.data()     , dst_mac.data(), 6);
        std::memcpy(frame.data() + 6 , src_mac_.data(), 6);
        uint16_t eth = htons(ETH_P_CUSTOM);
        std::memcpy(frame.data() + 12, &eth, 2);
        std::memcpy(frame.data() + 14, data, len);

        ssize_t sent = ::send(fd_, frame.data(), frame.size(), 0);
        if (sent < 0 || (size_t)sent != frame.size())
            throw std::runtime_error("send(): " + std::string(strerror(errno)));
    }

    // Recebe bruto (bloca até chegar algo ou erro)
    ssize_t recv_frame(uint8_t* buf, size_t buf_size) {
        return ::recv(fd_, buf, buf_size, 0);
    }

private:
    int fd_{-1};
    std::array<uint8_t,6> src_mac_{};
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Uso: " << argv[0] << " <interface> <dest-mac>\n";
        return 1;
    }

    try {
        RawSocket raw(argv[1]);
        auto dst_mac = parse_mac(argv[2]);

        // Payload de exemplo
        std::string msg = "Hello from C++17 raw protocol!";
        std::vector<uint8_t> payload(msg.begin(), msg.end());

        // Monta header
        PacketHeader hdr;
        hdr.type   = 1;                           // 1 = data
        hdr.length = htons((uint16_t)payload.size());
        hdr.seq    = htonl(1);
        hdr.crc16  = 0;

        // Calcula CRC sobre (type|length|seq) + payload
        std::vector<uint8_t> crcbuf(sizeof(hdr) + payload.size());
        std::memcpy(crcbuf.data(), &hdr, sizeof(hdr));
        std::memcpy(crcbuf.data()+sizeof(hdr), payload.data(), payload.size());
        hdr.crc16 = htons(crc16_ccitt(crcbuf.data(), crcbuf.size()));

        // Serializa header+payload
        std::vector<uint8_t> frame_data(sizeof(hdr) + payload.size());
        std::memcpy(frame_data.data(), &hdr, sizeof(hdr));
        std::memcpy(frame_data.data()+sizeof(hdr), payload.data(), payload.size());

        // Envia
        raw.send_frame(dst_mac, frame_data.data(), frame_data.size());

        // Obter MAC de origem do socket
        auto src_mac = raw.src_mac();
        std::cout << "Enviado: \"" << msg << "\" de " << format_mac(src_mac.data())
                  << " para " << format_mac(dst_mac.data()) << "\n";

        // Recebe e imprime o primeiro frame com nosso Ethertype
        uint8_t buf[1600];
        while (true) {
            ssize_t n = raw.recv_frame(buf, sizeof(buf));
            if (n <= 0) {
                std::cerr << "recv() erro: " << strerror(errno) << "\n";
                break;
            }
            uint16_t eth = ntohs(*reinterpret_cast<uint16_t*>(buf+12));
            if (eth != ETH_P_CUSTOM) continue;

            auto* rh = reinterpret_cast<PacketHeader*>(buf + 14);
            size_t plen = ntohs(rh->length);
            std::string rdata((char*)(buf+14+sizeof(PacketHeader)), plen);

            // Extrair MACs do frame Ethernet (destino nos primeiros 6 bytes, origem nos próximos 6)
            std::string src_mac_str = format_mac(buf + 6);  // MAC origem
            std::string dst_mac_str = format_mac(buf);      // MAC destino

            std::cout << "Recebido: \"" << rdata << "\" de " << src_mac_str
                      << " para " << dst_mac_str
                      << " (type=" << int(rh->type) << " seq=" << ntohl(rh->seq) << ")\n";
            break;
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Erro: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}