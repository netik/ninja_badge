#ifndef RFIO_H
#define RFIO_H

#include <info.h>
#include <lock.h>

void rfio_run(void);
volatile packet_t *swi_rfreceive(unsigned int starttime, unsigned int length);
void swi_sendpacket(u_int32_t badgeid, packet_t *packet, unsigned int tries);
int rfio_mask_matches(u_int32_t mask, volatile packet_t *p);
void rfio_watchdog(void);
void rfio_init(void);
void rfio_queue_add(volatile packet_t *p);
void rfio_txqueue_add(volatile packet_t *p);
volatile packet_t *rfio_check_queue(u_int32_t mask);
volatile packet_t *rfio_check_txqueue(void);
void rfio_set_mask(u_int32_t mask);
void grant_item(u_int32_t badgeid, packet_t *pbuf, unsigned int item, unsigned int count);

#define IS_VALID_PACKET_TYPE(x) (   \
                                    ((x) == PACKET_TYPE_BEACON)             \
                                    || ((x) == PACKET_TYPE_STARTFIGHT)      \
                                    || ((x) == PACKET_TYPE_STARTFIGHTACK)   \
                                    || ((x) == PACKET_TYPE_YOUGO)   \
                                    || ((x) == PACKET_TYPE_IMDEAD)   \
                                    || ((x) == PACKET_TYPE_FIGHTTURN)   \
                                    || ((x) == PACKET_TYPE_GRANT)   \
                                )

#define PACKET_TYPE_BEACON 0x28
#define PACKET_TYPE_STARTFIGHT 0x37
#define PACKET_TYPE_STARTFIGHTACK 0x45
#define PACKET_TYPE_YOUGO 0x69
#define PACKET_TYPE_IMDEAD 0x11
#define PACKET_TYPE_FIGHTTURN 0xF1
#define PACKET_TYPE_GRANT 0xEE

#define TXSTATUS_SUCCESS 1

#define CHILL_NOTIFY        0x1
#define CHILL_STARTFIGHT    0x2
#define CHILL_BUTTON        0x4
#define CHILL_TIMEOUT       0x8
#define CHILL_FIGHT         0x10
#define RET_TIMEOUT 0
#define RET_PACKET 1
#define RET_INVALID 2
#define RET_BUTTON 3

typedef 
struct rfio_yougo {
   union {
        u_int8_t initbyte;
        struct {
            unsigned char frame_type : 3;    // should be 0
            unsigned char mbo : 1;
            unsigned char mbz : 4;
        }__attribute__((packed));
    }__attribute__((packed));
    u_int8_t packettype;                    // should be PACKET_TYPE_YOUGO
    u_int8_t seqno;                         // sequence number
    u_int32_t badgeid;                    // badgeid
    u_int16_t rxoffset_scaled;            // rx offset in symbols, divided by 100
    u_int16_t period_scaled;              // beacon period in symbols, divided by 100
    u_int32_t dest_badgeid;               // destination
} __attribute__((packed)) rfio_yougo;

typedef 
struct rfio_fightturn {
   union {
        u_int8_t initbyte;
        struct {
            unsigned char frame_type : 3;    // should be 0
            unsigned char mbo : 1;
            unsigned char mbz : 4;
        }__attribute__((packed));
    }__attribute__((packed));
    u_int8_t packettype;                    // should be PACKET_TYPE_FIGHTTURN
    u_int8_t seqno;                         // sequence number
    u_int32_t badgeid;                      // badgeid
    u_int16_t rxoffset_scaled;              // rx offset in symbols, divided by 100
    u_int16_t period_scaled;                // beacon period in symbols, divided by 100
    u_int32_t dest_badgeid;                 // destination
    u_int16_t hithp;                        // total HP damage
    u_int16_t myhp;                         // sender's current HP
    u_int16_t rbboost;                      // red bull usage boost
    u_int16_t shuriken;                     // shuriken damage
    union {
        u_int8_t flagbyte;
        struct {
            unsigned char crit_hit : 1;
            unsigned char dodged : 1;
        } __attribute__((packed));
    } __attribute__((packed));
} __attribute__((packed)) rfio_fightturn;

typedef 
struct rfio_imdead {
   union {
        u_int8_t initbyte;
        struct {
            unsigned char frame_type : 3;    // should be 0
            unsigned char mbo : 1;
            unsigned char mbz : 4;
        }__attribute__((packed));
    }__attribute__((packed));
    u_int8_t packettype;                    // should be PACKET_TYPE_IMDEAD
    u_int8_t seqno;                         // sequence number
    u_int32_t badgeid;                    // badgeid
    u_int16_t rxoffset_scaled;            // rx offset in symbols, divided by 100
    u_int16_t period_scaled;              // beacon period in symbols, divided by 100
    u_int32_t dest_badgeid;               // destination
} __attribute__((packed)) rfio_imdead;

typedef 
struct rfio_grant {
   union {
        u_int8_t initbyte;
        struct {
            unsigned char frame_type : 3;    // should be 0
            unsigned char mbo : 1;
            unsigned char mbz : 4;
        }__attribute__((packed));
    }__attribute__((packed));
    u_int8_t packettype;                    // should be PACKET_TYPE_GRANT
    u_int8_t seqno;                         // sequence number
    u_int32_t badgeid;                      // badgeid
    u_int16_t rxoffset_scaled;              // rx offset in symbols, divided by 100
    u_int16_t period_scaled;                // beacon period in symbols, divided by 100
    u_int32_t dest_badgeid;                 // destination
    u_int8_t item;                          // item id granted
    u_int32_t itemnonce;                    // item nonce
} __attribute__((packed)) rfio_grant;


typedef 
struct rfio_startfight {
   union {
        u_int8_t initbyte;
        struct {
            unsigned char frame_type : 3;    // should be 0
            unsigned char mbo : 1;
            unsigned char mbz : 4;
        }__attribute__((packed));
    }__attribute__((packed));
    u_int8_t packettype;                    // should be STARTFIGHT or STARTFIGHTACK
    u_int8_t seqno;                         // sequence number
    u_int32_t badgeid;                    // badgeid
    u_int16_t rxoffset_scaled;            // rx offset in symbols, divided by 100
    u_int16_t period_scaled;              // beacon period in symbols, divided by 100
    u_int32_t dest_badgeid;               // destination

    union {
        u_int8_t abyte;
        struct {
            unsigned char playerlvl : 4;     // player level
            unsigned char namelen : 4;       // name length
        }__attribute__((packed));
    }__attribute__((packed));
    u_int16_t hp;
    union {
        u_int32_t various;
        struct {
            unsigned char shuriken_count : 4;
            unsigned char redbull_count : 4;
            unsigned int item_mask : 10;
            unsigned char is_ninja : 1;     
            unsigned char is_boss : 4;
        }__attribute__((packed));
    }__attribute__((packed));
    char name[0];
} __attribute__((packed)) rfio_startfight;


typedef
struct rfio_beacon {
    union {
        u_int8_t initbyte;
        struct {
            unsigned char frame_type : 3;    // should be 0
            unsigned char mbo : 1;
            unsigned char mbz : 4;
        }__attribute__((packed));
    }__attribute__((packed));
    u_int8_t packettype;                    // should be 0x28
    u_int8_t seqno;                         // sequence number
    u_int32_t badgeid;                    // badgeid
    u_int16_t rxoffset_scaled;            // rx offset in symbols, divided by 100
    u_int16_t period_scaled;              // beacon period in symbols, divided by 100
    union {
        u_int8_t abyte;
        struct {
            unsigned char playerlvl : 4;     // player level
            unsigned char namelen : 4;       // name length
        }__attribute__((packed));
    }__attribute__((packed));
    union {
        u_int16_t playerstate;
        struct {        // sync up with nearby_user
            unsigned char is_ninja : 1;    
            unsigned char is_boss : 4;
            unsigned int item_mask : 10;
        }__attribute__((packed));
    }__attribute__((packed));
    char name[0];
} __attribute__((packed)) rfio_beacon;

#define NNEARBY 16
extern nearby_user nearbys[NNEARBY];
extern lock_t nearbyslock;

#endif /* RFIO_H */
