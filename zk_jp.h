// zk_jp.h — Japanese text used in the world (shop signs, plates, broadcasts, notices, manga).
// tools/jpfont.py subsets Noto Sans CJK JP to exactly the characters in this file -> Data/UI/jp.ttf
static const char *jpShop[] = {"ラーメン", "居酒屋", "薬局", "本屋", "カラオケ", "喫茶店", "質屋", "寿司", "パチンコ", "クリーニング", "歯科医院", "電器店",
                               "理容室", "花屋", "酒屋", "古着屋", "定食屋", "銭湯", "写真館", "不動産", "ゲームセンター", "たばこ", "蕎麦", "焼肉"};
#define JP_NSHOP ((int)(sizeof jpShop / sizeof jpShop[0]))
static const char *jpKonbini = "コンビニ 24時間";
static const char *jpKoban = "交番";
static const char *jpStation = "霧町駅";
static const char *jpStationSub = "きりまち  KIRIMACHI";
static const char *jpTunnel[] = {"湾岸トンネル", "南浜トンネル", "北山トンネル", "旧霧峠隧道"};
static const char *jpSeal = "立入禁止";
static const char *jpSealSub = "危険 この先 崩落";
static const char *jpBcast[] = {"明るい緑の未来", "すべての市民に|きれいな水を", "落ち着いて|行動してください", "屋内に|留まってください",
                                "エコタワーで暮らそう|2006", "環境省は|あなたと共に", "笑顔で、|ご近所さん！", "救援は|向かっています", "深呼吸を。|すべて順調です。"};
#define JP_NBCAST ((int)(sizeof jpBcast / sizeof jpBcast[0]))
static const char *jpTicker = "環境省 ＊ 水質：良好 ＊ 空気：良好 ＊ 追って通知があるまで屋内に留まってください ＊ ";
static const char *jpNoSignal = "信号なし";
static const char *jpManga[] = {"…まだ誰かいるのか？", "逃げろ！", "あれは人間じゃない", "トンネルの向こうに…", "聞こえる？", "ここは安全だ",
                                "水がもうない", "夜が来る", "お札を貼れ！", "ドン", "ギャアア", "ゴゴゴゴ", "ザッ", "ドクン", "第三話", "残光"};
#define JP_NMANGA ((int)(sizeof jpManga / sizeof jpManga[0]))
static const char *jpVend = "つめた〜い";
static const char *jpSubway = "地下鉄";
static const char *jpSubwaySub = "霧町線  出入口";
static const char *jpMapTitle = "霧町  地図";
enum { SGN_KONBINI = JP_NSHOP, SGN_KOBAN, SGN_STATION, SGN_SUBWAY, SGN_TUN0, SGN_TUN1, SGN_TUN2, SGN_TUN3, SGN_SEAL, SGN_N };
