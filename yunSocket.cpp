#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <chrono>
#include <thread>
#include <iostream>

//定義I2C暫存器位置
#define I2C_addr 0x2A
#define I2C_forward 0x01
#define I2C_back 0x11
#define I2C_stop 0x00
#define I2C_LED_FULL 0x0FF0
#define I2C_LED_ZERO 0x0000

#define REG_left_rear 0x03
#define REG_right_rear 0x04
#define REG_LED 0x05


using namespace std;

typedef struct my_op{
	int cmd = -1;
	char op[10] = {0};
}My_op;

My_op operation;

//itank的操縱涵式
void forward(int file_i2c);
void back(int file_i2c);
void left_turn(int file_i2c);
void right_turn(int file_i2c);
void car_stop(int file_i2c);
void led_light(int file_i2c);
void led_darken(int file_i2c);

int main(int argc , char *argv[])
{
		int file_i2c;
		char *filename=(char *)"/dev/i2c-1";

		char inputBuffer[256] = {};
    int sockfd = 0, sClient = 0;

		//open the i2c bus
		if((file_i2c=open(filename,O_RDWR))<0)
		{
			printf("Failed to openthei2c bus\n\r");
			return 0;
		}
		else
			printf("open /dev/i2c-1 ok!\r\n");

		//setup slave address
		int addr=0x2A;
		if(ioctl(file_i2c,I2C_SLAVE,addr)<0)
		{
			printf("Failed to acquire bus acess and/or talk to slave");
			return 0;
		}
		else
			printf("setup slaveaddress to 0x2A ok!\r\n");

		sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    //socket的連線
    struct sockaddr_in serverInfo,clientInfo;
    int addrlen = sizeof(clientInfo);

		//設定socket的初值
    bzero(&serverInfo,sizeof(serverInfo));
    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(5487);

		//啟動監聽
    bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
		listen(sockfd,5);

		int getop;
		char op[10];

    while(1){
				printf("Wait connect...");
				socklen_t sosize = sizeof(addrlen);
				cout << endl;
        sClient = accept(sockfd,(struct sockaddr*) &clientInfo, (socklen_t*)&sosize);
				printf("Accept one connection：%s \r\n", inet_ntoa(clientInfo.sin_addr));
				while(1){
					memset(inputBuffer, 0, sizeof(inputBuffer));
					memset(operation.op, 0, sizeof(operation.op));
					//接收資料
					recv(sClient,inputBuffer,sizeof(inputBuffer),0);
					printf("Get:%s\n",inputBuffer);
					getop = 0;

					//拆解json字串
					for(int i = 0; inputBuffer[i]; i++){
						if(inputBuffer[i] == '\"' && getop == 0){
							for(int j = i+1, tmp = 0; inputBuffer[j]; j++){
								if(inputBuffer[j] == '\"'){
									i = j;
									getop = 1;
									break;
								}
								operation.op[tmp++] = inputBuffer[j];
							}
						}

						if(getop == 1 && inputBuffer[i] == ':'){
							getop == 0;
							operation.cmd = inputBuffer[i+1] - '0';
							break;
						}
						if(inputBuffer[i] == '}') break;
					}
					cout << "OPT: " << operation.op << endl;
					cout << "CMD: " << operation.cmd << endl;

					//判斷Command的功能
					if(strcmp(operation.op, "opt") == 0){
						switch (operation.cmd) {
							case 1:
								std::cout << "Forward" << '\n';
								forward(file_i2c);
								break;
							case 2:
								std::cout << "Backward" << '\n';
								back(file_i2c);
								break;
							case 3:
								std::cout << "Left" << '\n';
								left_turn(file_i2c);
								break;
							case 4:
								std::cout << "Right" << '\n';
								right_turn(file_i2c);
								break;
							case 5:
								std::cout << "Stop" << '\n';
							default:
								car_stop(file_i2c);
						}
					}
				}

				close (sClient);
    }
    return 0;
}

void forward(int file_i2c)
{
	i2c_smbus_write_word_data(file_i2c, REG_left_rear, I2C_forward);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	i2c_smbus_write_word_data(file_i2c, REG_right_rear, I2C_forward);
}

void back(int file_i2c)
{
	i2c_smbus_write_word_data(file_i2c, REG_left_rear, I2C_back);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	i2c_smbus_write_word_data(file_i2c, REG_right_rear, I2C_back);
}

void left_turn(int file_i2c)
{
	i2c_smbus_write_word_data(file_i2c, REG_left_rear, I2C_back);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	i2c_smbus_write_word_data(file_i2c, REG_right_rear, I2C_forward);
}

void right_turn(int file_i2c)
{
	i2c_smbus_write_word_data(file_i2c, REG_left_rear, I2C_forward);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	i2c_smbus_write_word_data(file_i2c, REG_right_rear, I2C_back);
}

void car_stop(int file_i2c)
{
	i2c_smbus_write_word_data(file_i2c, REG_left_rear, I2C_stop);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	i2c_smbus_write_word_data(file_i2c, REG_right_rear, I2C_stop);
}

void led_light(int file_i2c)
{
	i2c_smbus_write_word_data(file_i2c, REG_LED, I2C_LED_FULL);
}

void led_darken(int file_i2c)
{
	i2c_smbus_write_word_data(file_i2c, REG_LED, I2C_LED_FULL);
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	i2c_smbus_write_word_data(file_i2c, REG_LED, I2C_LED_ZERO);
}
