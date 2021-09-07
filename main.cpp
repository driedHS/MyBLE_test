#include "mbed.h"

#define CHARA_H 0x000E

BufferedSerial PC(D1, D0, 9600);
BufferedSerial BLE(A0, A1, 115200);

PwmOut led_g(D13);
PwmOut led_r(D12);
InterruptIn   sw0(PC_13);
InterruptIn   sw1(D4, PullUp);
InterruptIn   sw2(D5, PullUp);
InterruptIn   sw3(D6, PullUp);
InterruptIn   sw4(D7, PullUp);

Thread th_receive;
Thread th_sw;

Mutex mutex;

EventFlags event_ble;
#define READFLAG (1 << 0)
EventFlags event_sw;
#define SW0FLAG (1 << 0)
#define SW1FLAG (1 << 1)
#define SW2FLAG (1 << 2)
#define SW3FLAG (1 << 3)
#define SW4FLAG (1 << 4)

typedef struct {
    int handle;
    int value;
    int count;
} mail_t;
Mail<mail_t, 16> mail_box;

int charToInt(char myString){
    if(myString >= '0' && myString <= '9')
        return myString - '0';
    if(myString >= 'A' && myString <= 'F')
        return myString - 'A' + 0xA;
    return -1;
}

int stringToInt(int* p, char myString[]){
    int data = 0;
    int i = 0;
    while(charToInt(myString[i]) != -1){
        data = (data << 4) | charToInt(myString[i++]);
    }
    if(i == 0)
        return -1;
    *p = data;
    return 0;
}


void ble_get_state(){
    if(BLE.readable()){
        event_ble.set(READFLAG);
    }
}

void sw0_fall(){event_sw.set(SW0FLAG);}
void sw1_fall(){event_sw.set(SW1FLAG);}
void sw2_fall(){event_sw.set(SW2FLAG);}
void sw3_fall(){event_sw.set(SW3FLAG);}
void sw4_fall(){event_sw.set(SW4FLAG);}

void setup(){
    ThisThread::sleep_for(1s);
    BLE.write("SF,1\r\n", 6);
    ThisThread::sleep_for(50ms);
    BLE.write("S-,mbed-os\r\n", 12);
    ThisThread::sleep_for(50ms);
    BLE.write("SDM,mbed-os\r\n", 13);
    ThisThread::sleep_for(50ms);
    BLE.write("SS,00000001\r\n", 13);
    ThisThread::sleep_for(50ms);
    BLE.write("SR,24000000\r\n", 13);
    ThisThread::sleep_for(50ms);
    BLE.write("PZ\r\n", 4);
    ThisThread::sleep_for(50ms);
    BLE.write("PS,0ba61fc534b64d73a5621051c64e6665\r\n", 37);
    ThisThread::sleep_for(50ms);
    BLE.write("PC,0ad84fb853a2402e9e3c048f88367381,18,04\r\n", 43);
    ThisThread::sleep_for(50ms);
    BLE.write("PC,49a99bde9c9a48e3b85262439e069af1,12,01\r\n", 43);
    ThisThread::sleep_for(50ms);
    BLE.write("R,1\r\n", 5);
    ThisThread::sleep_for(50ms);
    BLE.write("+\r\n", 3);
}

/***************************
 * Thread Functions
 * - ble_receive    ble.readableなら起床 文字列取得
 ***************************/
void ble_receive(){
    PC.write(" > start ble_receive\r\n",22);
    char data[32];
    int count = 0;
    for(;;){
        event_ble.wait_any(READFLAG, osWaitForever, true);
        char buf;
        BLE.read(&buf, 1);
        mutex.lock();
        PC.write(&buf, 1);
        mutex.unlock();
        
        data[count++] = buf;
        if(buf == '\n'){
            if(data[0] == 'W' && data[1] == 'V'){   //WV,HAND,VALU
                mail_t *mail = mail_box.try_alloc();
                
                if(stringToInt(&(mail->handle),&data[3])==0 && stringToInt(&(mail->value),&data[8])==0){
                    mail->count  = count;
                    mail_box.put(mail);
                }
            }
            count = 0;
        }
        
    }
}

void sw_response(){
    for(;;){
        
        unsigned int read_flag = event_sw.wait_any(0b11111, osWaitForever, true);
        ThisThread::sleep_for(100ms);
        
        if((read_flag & SW0FLAG) > 0){
            BLE.write("LS\r\n", 4);
            PC.write("LS\r\n", 4);
        }
        if((read_flag & SW1FLAG) > 0){
            BLE.write("SUW,49a99bde9c9a48e3b85262439e069af1,01\r\n", 41);
            PC.write("SUW,49a99bde9c9a48e3b85262439e069af1,01\r\n", 41);
        }
        if((read_flag & SW2FLAG) > 0){
            BLE.write("SUW,49a99bde9c9a48e3b85262439e069af1,02\r\n", 41);
            PC.write("SUW,49a99bde9c9a48e3b85262439e069af1,02\r\n", 41);
        }
        if((read_flag & SW3FLAG) > 0){
            BLE.write("SUW,49a99bde9c9a48e3b85262439e069af1,03\r\n", 41);
            PC.write("SUW,49a99bde9c9a48e3b85262439e069af1,03\r\n", 41);
        }
        if((read_flag & SW4FLAG) > 0){
            BLE.write("SUW,49a99bde9c9a48e3b85262439e069af1,04\r\n", 41);
            PC.write("SUW,49a99bde9c9a48e3b85262439e069af1,04\r\n", 41);
        }
        event_sw.set(0);
    }
}

int main(){
    PC.write("start\r\n",7);
    BLE.sigio(callback(ble_get_state));             //BLEの Write or Read が起きた時に割込み
    th_receive.start(callback(ble_receive));        
    th_sw.start(callback(sw_response));
    th_sw.set_priority(osPriorityBelowNormal7);     //set new pri   :23
    sw0.fall(callback(sw0_fall));
    sw1.fall(callback(sw1_fall));
    sw2.fall(callback(sw2_fall));
    sw3.fall(callback(sw3_fall));
    sw4.fall(callback(sw4_fall));

    setup();
    
    for(;;){
        mail_t *mail = mail_box.try_get_for(Kernel::wait_for_u32_forever);
        mutex.lock();
        PC.write("got mail : ",11);
        mutex.unlock();
        if (mail != nullptr) {
            mutex.lock();
            PC.write("success\r\n",9);
            mutex.unlock();
            char printData[18];
            int han = mail->handle;
            int val = mail->value;
            mail_box.free(mail);
            
            sprintf(printData, "h:0x%04x\r\n", han);
            mutex.lock();
            PC.write(printData, 10);
            mutex.unlock();
            
            sprintf(printData, "v:0x%08x\r\n", val);
            mutex.lock();
            PC.write(printData, 14);
            mutex.unlock();
            
            if(han == CHARA_H){
                if(val >= 0){
                    led_g.write(val/100.0f);
                    led_r.write(0.0f);
                }
                else{
                    led_g.write(0.0f);
                    led_r.write(-1*val/100.0f);
                }
            }
        }
        else{
            PC.write("failure\r\n",9);
            led_g.write(0.0f);
            led_r.write(0.0f);
        }
    }
}