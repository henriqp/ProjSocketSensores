# Projeto - Integração dos Exemplos


## **Sumário**

* Introdução
* Requisitos
* Execução


## **Introdução**

Integrar o projeto de WiFi com GPIO com o projeto de Socket TCP, utilizando do exemplo de Socket TCP server. O objetivo final é ter um sistema que consiga monitorar o status do WiFi do ESP8266 através de um LED, utilizar um botão para tentar nova conexão de WiFi e, quando conectado, criar um servidor Socket TCP para que algum cliente (App de celular, por exemplo) possa realizar a conexão e requisitar a informação dos sensores.


## **Requisitos**

Pré-requisitos
	
* ESP8266 Toolchain
* ESP8266 RTOS SDK
* ESP8266 Python Packages

Componentes utilizados:

* WeMos D1 R2
* ESP8266
* Sensor de temperatura e umidade DHT11
* Sensor de distância HC-SR04
* Botão

Recursos utilizados:
-> Linguagem C

* GPIO Driver
* FreeRTOS Tasks
* FreeRTOS Event Groups
* FreeRTOS Queue
* FreeRTOS lwIP Sockets
* ESP WiFi
* ESP Non-volatile storage
* Biblioteca DHT & Ultrasonic

## **Execução**

### **Conectar**
	
Conecte o ESP8266 no PC em uma porta USB e verifique se a comunicação serial funciona.
O nome da porta será usado na configuração do projeto.


### **Configurar**
	
1. Através do terminal, vá até o diretorio do projeto.

```
cd ~/ProjSocketSensores
```

2. Em seguida, execute o comando:

```
make menuconfig
```

3. No menu de configuração, navegue até **_Serial flasher config_** -> **_Default serial port_** e digite o nome da
   porta usada.
   Salve as alterações utilizando a opção <Save> e volte ao menu principal através da opção <Exit>.

4. Em seguida, navegue até **_configuração do projeto_** e informe o SSID e senha da rede WiFi, escolha entre
   IPv4 e IPv6 e a porta do servidor TCP.
   Salve as alterações utilizando a opção <Save>.

Após realizar todas as configurações utilize a opção <Exit>.


### **Build e Flash**

Para compilar o aplicativo e todos os componentes e monitorar a execução do aplicativo utilizar o comando:

```
make flash monitor
```