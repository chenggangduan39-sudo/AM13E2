#include "qtk_asound_card.h"

typedef enum {
    QTK_ASOUND_CARD_S,
    QTK_ASOUND_CARD_E,
    QTK_ASOUND_CARD_ID_S,
    QTK_ASOUND_CARD_ID_E,
    QTK_ASOUND_DEVICE_ID_S,
    QTK_ASOUND_DEVICE_ID_E,
    QTK_ASOUND_CARD_NAME_S,
    QTK_ASOUND_CARD_NAME_E,
    QTK_ASOUND_NEXT,
} qtk_asound_card_t;

int qtk_asound_get_card() {
    qtk_asound_card_t state;
    int cards = -1;
    int i;
    char card[8], cardid[32], deviceid[32], cardname[32];
    char data[QTK_ASOUND_BUFSIZE], *s, *e;
    int len;
    FILE *fp;
    int b;

    fp = fopen(QTK_ASOUND_CARD_PATH, "rb");
    len = fread(data, 1, QTK_ASOUND_BUFSIZE, fp);
    fclose(fp);

    s = data;
    e = data + len;
    state = QTK_ASOUND_CARD_S;
    i = 0;
    b = 0;
    while (s < e) {
        switch (state) {
        case QTK_ASOUND_CARD_S:
            if (*s != ' ') {
                card[i++] = *s;
                state = QTK_ASOUND_CARD_E;
            }
            break;
        case QTK_ASOUND_CARD_E:
            if (*s == ' ') {
                card[i] = '\0';
                i = 0;
                state = QTK_ASOUND_CARD_ID_S;
            } else {
                card[i++] = *s;
            }
            break;
        case QTK_ASOUND_CARD_ID_S:
            if (*s != ' ') {
                cardid[i++] = *s;
                state = QTK_ASOUND_CARD_ID_E;
            }
            break;
        case QTK_ASOUND_CARD_ID_E:
            if (*s == ':') {
                cardid[i] = '\0';
                i = 0;
                state = QTK_ASOUND_DEVICE_ID_S;
            } else {
                cardid[i++] = *s;
            }
            break;
        case QTK_ASOUND_DEVICE_ID_S:
            if (*s != ' ') {
                deviceid[i++] = *s;
                state = QTK_ASOUND_DEVICE_ID_E;
            }
            break;
        case QTK_ASOUND_DEVICE_ID_E:
            if (*s == ' ') {
                deviceid[i] = '\0';
                i = 0;
                state = QTK_ASOUND_CARD_NAME_S;
            } else {
                deviceid[i++] = *s;
            }
            break;
        case QTK_ASOUND_CARD_NAME_S:
            if (*s != ' ' && *s != '-') {
                cardname[i++] = *s;
                state = QTK_ASOUND_CARD_NAME_E;
            }
            break;
        case QTK_ASOUND_CARD_NAME_E:
            if (*s == '\n') {
                cardname[i] = '\0';
                i = 0;
                state = QTK_ASOUND_NEXT;
                wtk_log_log(
                    glb_log,
                    "card = %s.cardname = %s.deviceid = %s.cardid = %s.", card,
                    cardid, deviceid, cardname);
#ifdef MTK8516
                if (strncmp(deviceid, "mt-snd-card", 11) == 0)
#else
                if (strncmp(deviceid, "USB-Audio", 9) == 0 &&
                    strncmp(cardname, "Qdreamer", 8) == 0)
#endif
                {
                    b = 1;
                    goto end;
                }
            } else {
                cardname[i++] = *s;
            }
            break;
        case QTK_ASOUND_NEXT:
            if (*s == '\n') {
                state = QTK_ASOUND_CARD_S;
            }
            break;
        }
        ++s;
    }
end:
    if (b) {
        cards = atoi(card);
    } else {
        wtk_log_log0(glb_log, "[ERROR]:Get asound card failed.\n");
    }
    return cards;
}
