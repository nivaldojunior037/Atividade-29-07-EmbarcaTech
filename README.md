# Atividade 29-07

Este é o repositório que armazena a tarefa solicitada no dia 22/07. Todos os arquivos necessários à execução já foram criados, de modo que basta seguir as instruções abaixo para executá-lo em seu dispositivo.

## Como Usar

1. Para acessar a atividade armazenada, clone este repositório para seu dispositivo executando o seguinte comando no terminal de um ambiente adequado, como o VS Code, após criar um repositório local: 
git clone https://github.com/nivaldojunior037/Atividade-29-07-EmbarcaTech

2. Após isso, importe como um projeto a pasta que contém os arquivos clonados em um ambiente como o VS Code e compile o código existente para que os demais arquivos necessários sejam criados em seu dispositivo.

3. Ao fim da compilação, será gerado um arquivo .uf2 na pasta build do seu repositório. Esse arquivo deve ser copiado para a BitDogLab em modo BOOTSEL para que ele seja corretamente executado. 

4. É necessário conectar um sensor MPU6050 ao terminal I2C0 da BitDogLab para que o código funcione adequadamente realizando as leituras de aceleração e velocidade. Além disso, também é necessário um cartão SD com adaptação SPI adequada para comunicação com a placa de desenvolvimento. 

### Como Executar o Código

1. Para executar o código, é necessário realizar a conexão entre a placa e o sensor e entre a placa e o cartão SD. Uma vez que o código esteja copiado para a BitDogLab, basta realizar o monitoramento serial em um ambiente adequado, como o terminal serial do VS Code ou o Putty, considerando baud rate de 115200. 

2. Os comandos listados abaixo servem para utilizar as funções do código por meio do terminal serial, de modo que basta digitar e enviar um deles que a ação correspondente é executada. 

**Atalhos de teclado no terminal (pressione apenas a tecla):**

| Tecla  | Função                                                           |
|--------|------------------------------------------------------------------|
| `a`    | Monta o cartão SD (`mount`)                                      |
| `b`    | Desmonta o cartão SD (`unmount`)                                 |
| `c`    | Lista os arquivos do cartão SD (`ls`)                            |
| `d`    | Lê e exibe o conteúdo do arquivo `data_mpu6050.csv`              |
| `e`    | Mostra o espaço livre no cartão SD (`getfree`)                   |
| `f`    | Captura amostras do sensor e salva no arquivo `data_mpu6050.csv` |
| `h`    | Exibe os comandos disponíveis (`help`)                           |

3. A placa emite feedback de acordo com as ações executadas. O buzzer emite sinais sonoros em caso de iniciação do processo de leitura e de formatação do cartão SD (respectivamente, um beep curto e dois beeps, sendo um de início e um de fim de formatação). O LED RGB central da placa emite sinais visuais para feedback, mostrando:

  3.1. Luz branca durante o processo de formatação;
  
  3.2. Luz azul durante o processo de leitura;
  
  3.3. Luz vermelha durante a desmontagem do cartão;
  
  3.4. Luz amarela durante a montagem do cartão;
  
  3.5. Luz roxa em caso de detecção de algum erro, como solicitação de leitura com o cartão desmontado;
  
  3.6. Luz verde ao fim do processo de leitura.

4. O display OLED também emite feedback, informando ao usuário o sensor utilizado e a ação em andamento. 

#### Link do vídeo

Segue o link do Drive com o vídeo onde é demonstrada a utilização do código: https://drive.google.com/file/d/1f49RcSOABUGbU-gaJw71qRk5zfW7oY3H/view?usp=sharing




