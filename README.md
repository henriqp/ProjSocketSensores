# Projeto - Integração dos Exemplos

![](https://lh3.googleusercontent.com/proxy/cJzAeRbH1F1_-NGB9k2z_xLnoZ2vssf1b5Y7UNlflydtwIMkh_4N5JamOZlQktenzLRb29Nwjio4XnBwHj0-ZiCgadJbO7MwlLOxgS2tDLI8taAge6s)

## **Sumário**

* Introdução
* Requisitos
* Execução
* Funcionamento


## **Introdução**

Integrar o projeto de WiFi com GPIO com o projeto de Socket TCP, utilizando do exemplo de Socket TCP server. O objetivo final é ter um sistema que consiga monitorar o status do WiFi do ESP8266 através de um LED, utilizar um botão para tentar nova conexão de WiFi e, quando conectado, criar um servidor Socket TCP para que algum cliente (App de celular, por exemplo) possa realizar a conexão e requisitar a informação dos sensores.


## **Requisitos**

### Pré-requisitos
	
* ESP8266 Toolchain
* ESP8266 RTOS SDK
* ESP8266 Python Packages

### Componentes utilizados:

* WeMos D1 R2
* ESP8266
* Sensor de temperatura e umidade DHT11
* Sensor de distância HC-SR04
* Botão

### Recursos utilizados:

#### Linguagem C

* GPIO Driver
* FreeRTOS Tasks
* FreeRTOS Event Groups
* FreeRTOS Queues
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
   
![serial](https://user-images.githubusercontent.com/74508536/99197360-ccc1b780-2770-11eb-8a6e-c981e5e57fd6.png)

Salve as alterações utilizando a opção *Save* e volte ao menu principal através da opção *Exit*.

4. Em seguida, navegue até **_configuração do projeto_** e informe o SSID e senha da rede WiFi, escolha entre
   IPv4 e IPv6 e a porta do servidor TCP.

![config](https://user-images.githubusercontent.com/74508536/99197387-f8dd3880-2770-11eb-9611-f08267241561.png)

Salve as alterações utilizando a opção *Save*.

Após realizar todas as configurações utilize a opção *Exit*.


### **Build e Flash**

Para compilar o aplicativo e todos os componentes e monitorar a execução do aplicativo utilizar o comando:

```
make flash monitor
```


## Funcionamento

### Aplicação

A aplicação inicializa o armazenamento não volátil e as configurações do WiFi do ESP8266. Em seguida, são criadas as
filas e as tarefas do FreeRTOS para conexão com WiFi, leituras dos sensores de temperatura e distância e inicialização
do servidor TCP.

* *task_sinalizarConexaoWifi*
* *task_reconectarWifi*
* *task_CriarTCPServer*
* *task_LerTemperaturaUmidade*
* *task_lerDistancia*

A função *event_handler* tem a finalidade de realizar a conexão com o WiFi local, sendo utilizados Event Groups
para indicar a situação da conexão para as demais tarefas e impedir que a criação do servidor TCP seja realizada
sem estar conectado com o WiFi local.

A task *task_sinalizarConexaoWifi* sinaliza através do *LED BUILDING* o status da conexão com o WiFi.
* 500ms - WiFi conectando
* 100ms - WiFi falhou
* 10ms - WiFi conectado

Quando ocorrer falha na conexão do WiFi, a tarefa *task_reconectarWifi* é desbloqueada para tentar nova conexão.
A tarefa de reconexão lê um botão que ao ser pressionado tenta reconectar novamento ao WiFi.

Ao se conectar ao WiFi é desbloqueado a task *task_CriarTCPServer* para criar o servidor TCP. Esta tarefa faz a
configuração do socket e espera pela conexão dos clientes. Quando um cliente se conecta, o socket aceita a conexão
e espera por mensagens vindas do cliente.

As tasks *task_LerTemperaturaUmidade* e *task_lerDistancia* leem as informações dos sensores conectados, salvando
os valores em uma estrutura que é enviada para a fila criada.

A task *task_CriarTCPServer* recebe os valores das tasks de leitura através da fila, então, é verificado a informação
que o usuário requisitou para enviar a respectiva informação.


### Cliente

O cliente se conecta com o servidor informando o IP e a porta do servidor. Ao se conectar, o usuário recebe um menu
com as opções de visualização.

O cliente informa a opção e espera a resposta do servidor.

![cliente](https://user-images.githubusercontent.com/74508536/99199985-8cb70080-2781-11eb-87cc-505ed656bab0.png)