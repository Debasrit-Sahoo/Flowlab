#include <windows.h>
#include <windivert.h>
#include <stdio.h>
#include <stdint.h>
#include "statetable.h"
#include "keybinds.h"
#include "limiter.h"

#define MAXBUF 0xFFFF

uint8_t port_policy_flag = 0;

#pragma pack(push, 1)

typedef struct {
    uint8_t  ver_ihl;
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_frag;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t checksum;
    uint32_t src;
    uint32_t dst;
} IPv4Header;

typedef struct {
    uint32_t vtc_flow;
    uint16_t payload_len;
    uint8_t  next_hdr;
    uint8_t  hop_limit;
    uint8_t  src[16];
    uint8_t  dst[16];
} IPv6Header;

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
} L4Header;

#pragma pack(pop)

static HANDLE g_thread = NULL;
HANDLE handle;
void divert_loop(void);
static volatile int g_running = 0;

static int build_rule_clause(const Rule *r, char *buf, int buf_size) {
    uint16_t start = (uint16_t)(r->port_range >> 16);
    uint16_t end   = (uint16_t)(r->port_range & 0xFFFF);

    uint8_t proto = r->rule & (FLAG_TCP | FLAG_UDP);
    uint8_t dir   = r->rule & (FLAG_UL  | FLAG_DL);

    // sub-clauses get collected here then joined with or
    char parts[4][128];
    int  part_count = 0;

    const char *protos[2]     = { "tcp",   "udp"   };
    uint8_t     proto_flags[2]= { FLAG_TCP, FLAG_UDP };
    const char *dirs[2]       = { "Src",  "Dst"   };
    uint8_t     dir_flags[2]  = { FLAG_UL, FLAG_DL };

    for (int pi = 0; pi < 2; pi++) {
        if (!(proto & proto_flags[pi])) continue;
        for (int di = 0; di < 2; di++) {
            if (!(dir & dir_flags[di])) continue;
            snprintf(parts[part_count++], 128,
                "%s.%sPort >= %u and %s.%sPort <= %u",
                protos[pi], dirs[di^port_policy_flag], start,
                protos[pi], dirs[di^port_policy_flag], end);
        }
    }

    if (part_count == 0) return 0;

    // join parts with or into buf
    int written = 0;
    for (int i = 0; i < part_count; i++) {
        written += snprintf(buf + written, buf_size - written,
            "%s%s%s",
            i > 0 ? " or " : "",
            part_count > 1 && i == 0 ? "(" : "",
            parts[i]);
    }
    if (part_count > 1) {
        written += snprintf(buf + written, buf_size - written, ")");
    }

    return written;
}

char *build_filter(void) {
    // edge case: any rule covers full port space, just capture everything
    for (int i = 0; i < g_table.rule_count; i++) {
        uint16_t start = (uint16_t)(g_table.rules[i].port_range >> 16);
        uint16_t end   = (uint16_t)(g_table.rules[i].port_range & 0xFFFF);
        if (start == 0 && end == 65535) {
            char *f = malloc(5);
            if (f) strcpy_s(f, 5, "true");
            return f;
        }
    }

    // each rule clause is at most ~4 parts * 128 chars + or + parens
    // allocate generously
    int buf_size = g_table.rule_count * 600;
    char *filter = malloc(buf_size);
    if (!filter) return NULL;

    int written = 0;
    for (int i = 0; i < g_table.rule_count; i++) {
        char clause[600];
        int len = build_rule_clause(&g_table.rules[i], clause, sizeof(clause));
        if (len == 0) continue;

        written += snprintf(filter + written, buf_size - written,
            "%s%s",
            written > 0 ? " or " : "",
            clause);
    }

    return filter;
}

static DWORD WINAPI divert_thread(LPVOID param) {
    (void)param;
    divert_loop();
    return 0;
}

int divert_init(void){
    char* filter = build_filter();
    if (!filter){return 1;}
    handle = WinDivertOpen(filter, WINDIVERT_LAYER_NETWORK, 1000, 0);

    if (handle == INVALID_HANDLE_VALUE) {
        printf("WinDivertOpen failed: %lu\n", GetLastError());
        free(filter);
        return 1;
    }

    printf("WinDivert opened with filter\n");
    free(filter);
    g_running = 1;
    HANDLE thread = CreateThread(NULL, 0, divert_thread, NULL, 0, NULL);
        if (!thread) {
            printf("CreateThread failed: %lu\n", GetLastError());
            return 1;
        }
        g_thread = thread;

    return 0;
}

static int ipv6_find_l4(uint8_t* pkt, UINT len, UINT* l4_offset, uint8_t* out_proto){
    if (len < 40){return -1;}
    uint8_t next = (uint8_t)pkt[6];
    UINT offset = 40;

    if (next == 17 || next ==6){
        *l4_offset = offset;
        *out_proto = next;
        return 0;
    }
    int hops = 0;
    while (1){
        if (hops++ > 6){return -4;}
        if ((next == 6 || next == 17)){
            if (offset + 4 > len){return -1;}
            *l4_offset = offset;
            *out_proto = next;
            return 0;
        }
        if (next == 59){return -1;}
        
        uint8_t hdr_len = 0;
        switch (next){
            case 44:
                if (offset + 8 > len){return -1;}

                if (((pkt[offset + 2] << 8) | pkt[offset+3]) >> 3){return -2;}

                next = pkt[offset];
                offset += 8;
                continue;

            case 51:
                if (offset + 2 > len){return -1;}
                hdr_len = (pkt[offset+1] + 2)*4;
                break;

            case 0:
            case 43:
            case 60:
                if (offset + 2 > len){return -1;}
                hdr_len = (pkt[offset+1] + 1)*8;
                break;

            default:
                return -3;
        }
    if (offset + hdr_len > len || (hdr_len < 8)){return -1;}

    next = pkt[offset];
    offset += hdr_len;
    }
}

void divert_loop(void){
    printf("LOOP THREAD SPAWNED SUCCESSFULLY\n");
    printf("POLICY = %s\n", port_policy_flag ? "REMOTE" : "LOCAL");
    uint8_t packet[MAXBUF];
    WINDIVERT_ADDRESS addr;
    UINT packet_len;
    limiters_init();
    while (g_running) {
        if (!WinDivertRecv(handle, packet, sizeof(packet), &packet_len, &addr)) {
            continue;
        }
        //Detect IP version
        if (packet_len == 0) continue;
        uint8_t version = packet[0] >> 4;
        UINT ip_len;
        uint8_t proto;

        //Minimal IP parsing
        if (version == 4) {
            if (packet_len < 20) goto forward;

            uint8_t ihl = packet[0] & 0x0F;
            if (ihl < 5) goto forward;

            ip_len = ihl * 4;
            if (packet_len < ip_len) goto forward;

            uint16_t frag = (packet[6] << 8) | packet[7];

            uint16_t frag_offset = frag & 0x1FFF;

            if (frag_offset != 0){goto forward;}

            proto = packet[9];
        } 
        else if (version == 6){

            UINT l4_offset;
            if (ipv6_find_l4((uint8_t*)packet, packet_len, &l4_offset, &proto) != 0){
                goto forward;
            }

            if (l4_offset < 40 || l4_offset > packet_len){goto forward;}
            ip_len = l4_offset;
        }
        else{
            goto forward;
        }
        //TCP/UDP
        if (proto != 6 && proto != 17) {
            goto forward;
        }
        if (proto == 6) { // TCP
            if (packet_len < ip_len + 20){goto forward;}
            
            uint8_t tcp_hlen = ((packet[ip_len + 12] >> 4) & 0xF) * 4;
            if (tcp_hlen < 20){goto forward;}
            if (packet_len < ip_len + tcp_hlen){goto forward;};
        }
        else{ // UDP
            if (packet_len < ip_len + 8){goto forward;}
        }

        uint8_t outbound_interpretation = port_policy_flag ^ (uint8_t)addr.Outbound;
        uint16_t src_port =
            ((uint16_t)packet[ip_len] << 8) |
            (uint16_t)packet[ip_len + 1];
        uint16_t dst_port =
            ((uint16_t)packet[ip_len + 2] << 8) |
            (uint16_t)packet[ip_len + 3];
        uint16_t relevant_port = outbound_interpretation ? src_port : dst_port;
        uint8_t state = g_state_table[relevant_port];
        
        if (!state){goto forward;}

        uint8_t proto_flag = (proto == 17) ? FLAG_UDP : FLAG_TCP;
        uint8_t dir_flag   = addr.Outbound ? FLAG_UL : FLAG_DL;
        uint8_t mask = proto_flag | dir_flag;
        
        if ((state & mask) != mask){goto forward;}

        uint8_t action = state >> ACTION_SHIFT;
        
        if (!action){goto forward;}

        if (action == 1){
            continue; // DROP
        }
        RateLimiter *l = NULL;
        for (int i = 0; i < g_limiter_count; i++) {
            if (relevant_port >= g_limiters[i].port_start && relevant_port <= g_limiters[i].port_end) {
                l = &g_limiters[i];
                break;
            }
        }
        if (l && !limiter_consume(l, packet_len)){
            continue;
        }  // drop
        goto forward;

    forward:
        WinDivertSend(handle, packet, packet_len, NULL, &addr);
    }
}

void divert_close(void) {
    g_running = 0;
    if (handle) {
        WinDivertShutdown(handle, WINDIVERT_SHUTDOWN_BOTH);
        WinDivertClose(handle);
        handle = NULL;
    }
    if (g_thread){
        CloseHandle(g_thread);
        g_thread = NULL;
    }
}