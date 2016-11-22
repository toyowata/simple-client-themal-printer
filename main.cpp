#include "mbed.h"
#include "security.h"
#include "easy-connect.h"
#include "simple-mbed-client.h"
#include "AS289R2.h"

Serial pc(USBTX, USBRX);
SimpleMbedClient client;
AS289R2 tp(D1);
DigitalOut led(LED1);
DigitalOut blinkLed(LED2);
InterruptIn btn(MBED_CONF_APP_BUTTON);

Semaphore updates(0);

void patternUpdated(string v) {
    pc.printf("New pattern: %s\n", v.c_str());
}

void lcdTextUpdated(string v) {
    if (v.length() > 60) {
        v.erase(v.begin() + 60, v.end());
    }
    pc.printf("New test is: %s\r\n", v.c_str());
    tp.printf("%s\r\r", v.c_str());
    blinkLed = !blinkLed;
}

SimpleResourceInt btn_count = client.define_resource("button/0/clicks", 0, M2MBase::GET_ALLOWED);
SimpleResourceString lcd_text = client.define_resource("lcd/0/text",
    "Ready.", M2MBase::GET_PUT_ALLOWED, true, &lcdTextUpdated);

void fall() {
    updates.release();
}

void toggleLed() {
    led = !led;
}

void registered() {
    pc.printf("Registered\r\n");
}
void unregistered() {
    pc.printf("Unregistered\r\n");
}

const char url[] = "https://developer.mbed.org/components/AS-289R2-Thermal-Printer-Shield/";
 
void print_demo() {
    tp.setDoubleSizeWidth();
    tp.printf("  AS-289R2\r\r");
    tp.clearDoubleSizeWidth();
 
    tp.printf("日本語文字列の印字テスト:24x24\r");
    tp.setKanjiFont(AS289R2::KANJI_16x16);
    tp.setANKFont(AS289R2::ANK_8x16);
    tp.printf("日本語文字列の印字テスト:16x16\r\r");
 
    tp.setKanjiFont(AS289R2::KANJI_DEFAULT);
    tp.setANKFont(AS289R2::ANK_DEFAULT);
    tp.setDoubleSizeWidth();
    tp.printf("ABCDEFG 0123456789\r");
    tp.clearDoubleSizeWidth();
 
    tp.setDoubleSizeHeight();
    tp.printf("ABCDEFG 0123456789\r");
    tp.clearDoubleSizeHeight();
 
    tp.putLineFeed(2);
 
    tp.setANKFont(AS289R2::ANK_8x16);
    tp.printf("8x16: Test 012345 ｱｲｳｴｵ\r\r");
    tp.setANKFont(AS289R2::ANK_12x24);
    tp.printf("12x24: Test 012345 ｱｲｳｴｵ\r\r");
    tp.setANKFont(AS289R2::ANK_16x16);
    tp.printf("16x16: Test 012345 ｱｲｳｴｵ\r\r");
    tp.setANKFont(AS289R2::ANK_24x24);
    tp.printf("24x24: Test 012345 ｱｲｳｴｵ\r\r");
    tp.putLineFeed(1);
 
    tp.setANKFont(AS289R2::ANK_8x16);
    tp.printf("QR\r");
    tp.printQRCode(AS289R2::QR_ERR_LVL_M, url);
    tp.printf("\r%s\r", url);
    tp.putLineFeed(2);
 
    tp.printf("UPC-A\r");
    tp.printBarCode(AS289R2::BCODE_UPC_A, "01234567890");
    tp.putLineFeed(4);
 
    tp.initialize();
}

Ticker t;

int main() {
    pc.baud(115200);

    tp.initialize();
    tp.putLineFeed(2);
 
    tp.printf("** Thermal Printer Shield **\r\r");
    lcdTextUpdated(static_cast<string>(lcd_text).c_str());

    btn.fall(&fall);

    //t.attach(&toggleLed, 1.0f);
//    client.define_function("led/0/play", &play);

    NetworkInterface* network = easy_connect(true);
    if (!network) {
        return 1;
    }

    struct MbedClientOptions opts = client.get_default_options();
    opts.ServerAddress = MBED_SERVER_ADDRESS;
    opts.DeviceType = "prn";
    bool setup = client.setup(opts, network);
    if (!setup) {
        pc.printf("Client setup failed\n");
        return 1;
    }

    client.on_registered(&registered);
    client.on_unregistered(&unregistered);

    while (1) {
        int v = updates.wait(25000);
        if (v == 1) {
            btn_count = btn_count + 1;
            pc.printf("Button count is now %d\n", static_cast<int>(btn_count));
            print_demo();
        }
        client.keep_alive();
    }
}
