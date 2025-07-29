// Inclusão de todas as bibliotecas necessárias ao código
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "hardware/rtc.h"

#include "ff.h"
#include "diskio.h"
#include "f_util.h"
#include "hw_config.h"
#include "my_debug.h"
#include "rtc.h"
#include "sd_card.h"
 
// Inclusão de constantes e variáveis globais utilizadas
#define I2C_PORT i2c0               // i2c0 pinos 0 e 1, i2c1 pinos 2 e 3
#define I2C_SDA 0                   // 0 ou 2
#define I2C_SCL 1                  // 1 ou 3
#define addr 0x68
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define endereco 0x3C
#define LED_VERDE 11
#define LED_VERMELHO 13
#define LED_AZUL 12
#define BOTAO_A 5
#define BOTAO_B 6
#define BUZZER 21
static volatile uint32_t ultimo_tempo = 0;
ssd1306_t ssd;  
static bool logger_enabled;
static const uint32_t period = 1000;
static absolute_time_t next_log_time;
static volatile bool error_presence = false;
static int cRxedChar = 'i';
static volatile bool botA = false;
static volatile bool botBmount = false;
static volatile bool botBunmount = false;
static volatile bool SD_mount = false;

static char filename[20] = "data_mpu6050.csv";

// Função de reset do MPU6050
static void mpu6050_reset() {

    uint8_t buf[] = {0x6B, 0x80};
    i2c_write_blocking(I2C_PORT, addr, buf, 2, false);
    sleep_ms(100); 
 
    buf[1] = 0x00;  
    i2c_write_blocking(I2C_PORT, addr, buf, 2, false); 
    sleep_ms(10); 
}
 
// Função de leitura do MPU6050
static void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp) {
 
    uint8_t buffer[6];
 
    uint8_t val = 0x3B;
    i2c_write_blocking(I2C_PORT, addr, &val, 1, true); 
    i2c_read_blocking(I2C_PORT, addr, buffer, 6, false);
 
    for (int i = 0; i < 3; i++) {
        accel[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
    }
 
    val = 0x43;
    i2c_write_blocking(I2C_PORT, addr, &val, 1, true);
    i2c_read_blocking(I2C_PORT, addr, buffer, 6, false);  
 
    for (int i = 0; i < 3; i++) {
        gyro[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);;
    }
 
    val = 0x41;
    i2c_write_blocking(I2C_PORT, addr, &val, 1, true);
    i2c_read_blocking(I2C_PORT, addr, buffer, 2, false); 
 
    *temp = buffer[0] << 8 | buffer[1];
}

// Função para acionamento do buzzer
void tocar_buzzer(int frequencia, int duracao)
{
    for (int i = 0; i < duracao * 1000; i += (1000000 / frequencia) / 2)
    {
        gpio_put(BUZZER, 1);
        sleep_us((1000000 / frequencia) / 2);
        gpio_put(BUZZER, 0);
        sleep_us((1000000 / frequencia) / 2);
    }
}

//Função para inicializar os periféricos da placa e demais estruturas utilizadas
void inicializar_perif(){

    // I2C do Display funcionando em 400Khz.
    i2c_init(I2C_PORT_DISP, 400 * 1000);

    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA_DISP);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL_DISP);                                        // Pull up the clock line                                                   // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT_DISP); // Inicializa o display
    ssd1306_config(&ssd);                                              // Configura o display
    ssd1306_send_data(&ssd);                                           // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Inicializa o I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa os LEDs centrais da placa
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_put(LED_VERDE, 0);
    gpio_init(LED_AZUL);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_put(LED_AZUL, 0);
    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);
    gpio_put(LED_VERMELHO, 0);

    // Inicializa o buzzer e os botões A e B
    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);
    gpio_put(BUZZER, 0);
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);
    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);

} 

static sd_card_t *sd_get_by_name(const char *const name)
{
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name))
            return sd_get_by_num(i);
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}
static FATFS *sd_get_fs_by_name(const char *name)
{
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name))
            return &sd_get_by_num(i)->fatfs;
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}

static void run_setrtc()
{
    const char *dateStr = strtok(NULL, " ");
    if (!dateStr)
    {
        printf("Missing argument\n");
        return;
    }
    int date = atoi(dateStr);

    const char *monthStr = strtok(NULL, " ");
    if (!monthStr)
    {
        printf("Missing argument\n");
        return;
    }
    int month = atoi(monthStr);

    const char *yearStr = strtok(NULL, " ");
    if (!yearStr)
    {
        printf("Missing argument\n");
        return;
    }
    int year = atoi(yearStr) + 2000;

    const char *hourStr = strtok(NULL, " ");
    if (!hourStr)
    {
        printf("Missing argument\n");
        return;
    }
    int hour = atoi(hourStr);

    const char *minStr = strtok(NULL, " ");
    if (!minStr)
    {
        printf("Missing argument\n");
        return;
    }
    int min = atoi(minStr);

    const char *secStr = strtok(NULL, " ");
    if (!secStr)
    {
        printf("Missing argument\n");
        return;
    }
    int sec = atoi(secStr);

    datetime_t t = {
        .year = (int16_t)year,
        .month = (int8_t)month,
        .day = (int8_t)date,
        .dotw = 0, // 0 is Sunday
        .hour = (int8_t)hour,
        .min = (int8_t)min,
        .sec = (int8_t)sec};
    rtc_set_datetime(&t);
}

// Função para formatação do cartão
static void run_format()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    /* Format the drive with default parameters */
    FRESULT fr = f_mkfs(arg1, 0, 0, FF_MAX_SS * 2);
    if (FR_OK != fr)
        printf("f_mkfs error: %s (%d)\n", FRESULT_str(fr), fr);
}

// Função para montar o cartão
static void run_mount()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_mount(p_fs, arg1, 1);
    if (FR_OK != fr)
    {
        printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg1);
    myASSERT(pSD);
    pSD->mounted = true;
    printf("Processo de montagem do SD ( %s ) concluído\n", pSD->pcName);
}

// Função para desmontar o cartão
static void run_unmount()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_unmount(arg1);
    if (FR_OK != fr)
    {
        printf("f_unmount error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg1);
    myASSERT(pSD);
    pSD->mounted = false;
    pSD->m_Status |= STA_NOINIT; // in case medium is removed
    printf("SD ( %s ) desmontado\n", pSD->pcName);
}
static void run_getfree()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    DWORD fre_clust, fre_sect, tot_sect;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_getfree(arg1, &fre_clust, &p_fs);
    if (FR_OK != fr)
    {
        printf("f_getfree error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    tot_sect = (p_fs->n_fatent - 2) * p_fs->csize;
    fre_sect = fre_clust * p_fs->csize;
    printf("%10lu KiB total drive space.\n%10lu KiB available.\n", tot_sect / 2, fre_sect / 2);
}
static void run_ls()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = "";
    char cwdbuf[FF_LFN_BUF] = {0};
    FRESULT fr;
    char const *p_dir;
    if (arg1[0])
    {
        p_dir = arg1;
    }
    else
    {
        fr = f_getcwd(cwdbuf, sizeof cwdbuf);
        if (FR_OK != fr)
        {
            printf("f_getcwd error: %s (%d)\n", FRESULT_str(fr), fr);
            return;
        }
        p_dir = cwdbuf;
    }
    printf("Directory Listing: %s\n", p_dir);
    DIR dj;
    FILINFO fno;
    memset(&dj, 0, sizeof dj);
    memset(&fno, 0, sizeof fno);
    fr = f_findfirst(&dj, &fno, p_dir, "*");
    if (FR_OK != fr)
    {
        printf("f_findfirst error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    while (fr == FR_OK && fno.fname[0])
    {
        const char *pcWritableFile = "writable file",
                   *pcReadOnlyFile = "read only file",
                   *pcDirectory = "directory";
        const char *pcAttrib;
        if (fno.fattrib & AM_DIR)
            pcAttrib = pcDirectory;
        else if (fno.fattrib & AM_RDO)
            pcAttrib = pcReadOnlyFile;
        else
            pcAttrib = pcWritableFile;
        printf("%s [%s] [size=%llu]\n", fno.fname, pcAttrib, fno.fsize);

        fr = f_findnext(&dj, &fno);
    }
    f_closedir(&dj);
}
static void run_cat()
{
    char *arg1 = strtok(NULL, " ");
    if (!arg1)
    {
        printf("Missing argument\n");
        return;
    }
    FIL fil;
    FRESULT fr = f_open(&fil, arg1, FA_READ);
    if (FR_OK != fr)
    {
        printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    char buf[256];
    while (f_gets(buf, sizeof buf, &fil))
    {
        printf("%s", buf);
    }
    fr = f_close(&fil);
    if (FR_OK != fr)
        printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
}

// Função para capturar dados do ADC e salvar no arquivo *.csv
void capture_sensor_data_and_save()
{
    printf("\nCapturando dados do MPU6050. Aguarde finalização...\n");
    FIL file;
    FRESULT res = f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS);
    char buffer[128];
    if (res != FR_OK)
    {
        printf("\n[ERRO] Não foi possível abrir o arquivo para escrita. Monte o Cartao.\n");
        error_presence = true;
        return;
    }
    char header[] = "amostra,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,temp\n";
    UINT bw;
    res = f_write(&file, buffer, strlen(buffer), &bw);
    for (int i = 0; i < 128; i++)
    {
        error_presence = false;
        int16_t accel[3], gyro[3], temp;
        mpu6050_read_raw(accel, gyro, &temp);
        char buffer[50];
        sprintf(buffer, "%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n", i + 1, accel[0]/16384.0f, accel[1]/16384.0f, accel[2]/16384.0f, gyro[0]/131.0f, gyro[1]/131.0f, gyro[2]/131.0f);
        UINT bw;
        res = f_write(&file, buffer, strlen(buffer), &bw);
        if (res != FR_OK)
        {
            printf("[ERRO] Não foi possível escrever no arquivo. Monte o Cartao.\n");
            f_close(&file);
            error_presence = true;
            return;
        }
        sleep_ms(100);
    }
    error_presence = false;
    f_close(&file);
    printf("\nDados do ADC salvos no arquivo %s.\n\n", filename);
}

// Função para ler o conteúdo de um arquivo e exibir no terminal
void read_file(const char *filename)
{
    FIL file;
    FRESULT res = f_open(&file, filename, FA_READ);
    if (res != FR_OK)
    {
        printf("[ERRO] Não foi possível abrir o arquivo para leitura. Verifique se o Cartão está montado ou se o arquivo existe.\n");
        error_presence = true;

        return;
    }
    error_presence = false;
    char buffer[128];
    UINT br;
    printf("Conteúdo do arquivo %s:\n", filename);
    printf("amostra,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,temp\n");
    while (f_read(&file, buffer, sizeof(buffer) - 1, &br) == FR_OK && br > 0)
    {
        buffer[br] = '\0';
        printf("%s", buffer);
    }
    f_close(&file);
    printf("\nLeitura do arquivo %s concluída.\n\n", filename);
}

// Função para exibir as funções disponíveis
static void run_help()
{
    printf("\nComandos disponíveis:\n\n");
    printf("Digite 'a' para montar o cartão SD\n");
    printf("Digite 'b' para desmontar o cartão SD\n");
    printf("Digite 'c' para listar arquivos\n");
    printf("Digite 'd' para mostrar conteúdo do arquivo\n");
    printf("Digite 'e' para obter espaço livre no cartão SD\n");
    printf("Digite 'f' para capturar dados do MPU6050 e salvar no arquivo\n");
    printf("Digite 'g' para formatar o cartão SD\n");
    printf("Digite 'h' para exibir os comandos disponíveis\n");
    printf("\nEscolha o comando:  ");
}

typedef void (*p_fn_t)();
typedef struct
{
    char const *const command;
    p_fn_t const function;
    char const *const help;
} cmd_def_t;

static cmd_def_t cmds[] = {
    {"setrtc", run_setrtc, "setrtc <DD> <MM> <YY> <hh> <mm> <ss>: Set Real Time Clock"},
    {"format", run_format, "format [<drive#:>]: Formata o cartão SD"},
    {"mount", run_mount, "mount [<drive#:>]: Monta o cartão SD"},
    {"unmount", run_unmount, "unmount <drive#:>: Desmonta o cartão SD"},
    {"getfree", run_getfree, "getfree [<drive#:>]: Espaço livre"},
    {"ls", run_ls, "ls: Lista arquivos"},
    {"cat", run_cat, "cat <filename>: Mostra conteúdo do arquivo"},
    {"help", run_help, "help: Mostra comandos disponíveis"}};

static void process_stdio(int cRxedChar)
{
    static char cmd[256];
    static size_t ix;

    if (!isprint(cRxedChar) && !isspace(cRxedChar) && '\r' != cRxedChar &&
        '\b' != cRxedChar && cRxedChar != (char)127)
        return;
    printf("%c", cRxedChar); // echo
    stdio_flush();
    if (cRxedChar == '\r')
    {
        printf("%c", '\n');
        stdio_flush();

        if (!strnlen(cmd, sizeof cmd))
        {
            printf("> ");
            stdio_flush();
            return;
        }
        char *cmdn = strtok(cmd, " ");
        if (cmdn)
        {
            size_t i;
            for (i = 0; i < count_of(cmds); ++i)
            {
                if (0 == strcmp(cmds[i].command, cmdn))
                {
                    (*cmds[i].function)();
                    break;
                }
            }
            if (count_of(cmds) == i)
                printf("Command \"%s\" not found\n", cmdn);
        }
        ix = 0;
        memset(cmd, 0, sizeof cmd);
        printf("\n> ");
        stdio_flush();
    }
    else
    {
        if (cRxedChar == '\b' || cRxedChar == (char)127)
        {
            if (ix > 0)
            {
                ix--;
                cmd[ix] = '\0';
            }
        }
        else
        {
            if (ix < sizeof cmd - 1)
            {
                cmd[ix] = cRxedChar;
                ix++;
            }
        }
    }
}

// Função para utilização de botões por meio de interrupções e debounce
static void gpio_irq_handler(uint gpio, uint32_t events){
    uint32_t tempo_atual = to_us_since_boot(get_absolute_time());

    if(tempo_atual - ultimo_tempo > 200000){
        if(!gpio_get(BOTAO_A)){
            if (cRxedChar != 'f'){
                botA = true;
            } 
            ultimo_tempo = tempo_atual;
        } else if (!gpio_get(BOTAO_B)){
            if (cRxedChar != 'a' && !SD_mount){
                botBmount = true;
            } else if(cRxedChar != 'b' && SD_mount){
                botBunmount = true;
            }
            ultimo_tempo = tempo_atual;
        }
    }
}

// Função principal
int main()
{
    // Inicialização de estruturas
    stdio_init_all();
    inicializar_perif();
    time_init();
    char sd_status[20] = "Aguardando";
    bool cor = true;
    // Habilitação de interrupções
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    bi_decl(bi_2pins_with_func(I2C_SDA, I2C_SCL, GPIO_FUNC_I2C));
    printf("Antes do reset MPU...\n");
    mpu6050_reset();

    printf("FatFS SPI example\n");
    printf("\033[2J\033[H"); 
    printf("\n> ");
    stdio_flush();
    run_help();

    while (true)
    {
        strcpy(sd_status, "Aguardando");
        //  Atualiza o conteúdo do display com animações
        ssd1306_fill(&ssd, !cor);                           // Limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);       // Desenha um retângulo
        ssd1306_line(&ssd, 3, 32, 123, 32, cor);            // Desenha uma linha
        ssd1306_draw_string(&ssd, "Sensor MPU6050", 8, 8);  // Desenha uma string
        ssd1306_draw_string(&ssd, "Status ativo", 12, 20);  // Desenha uma string
        ssd1306_draw_string(&ssd, sd_status, 14, 46);       // Desenha uma string
        ssd1306_send_data(&ssd);                            // Atualiza o display

        // Avalia o caractere inserido
        cRxedChar = getchar_timeout_us(0);
        if (PICO_ERROR_TIMEOUT != cRxedChar)
            process_stdio(cRxedChar);

        if (cRxedChar == 'a' || botBmount) // Monta o SD card se pressionar 'a'
        {
            printf("\nMontando o SD...\n");
            gpio_put(LED_VERDE, 1);
            gpio_put(LED_VERMELHO, 1);
            gpio_put(LED_AZUL, 0);
            strcpy(sd_status, "Montando");
            run_mount();
            printf("\nEscolha o comando (h = help):  ");
            botBmount = false;
            SD_mount = true;
        }
        if (cRxedChar == 'b' || botBunmount) // Desmonta o SD card se pressionar 'b'
        {
            printf("\nDesmontando o SD. Aguarde...\n");
            gpio_put(LED_VERDE, 0);
            gpio_put(LED_VERMELHO, 1);
            gpio_put(LED_AZUL, 0);
            strcpy(sd_status, "Desmontando");
            run_unmount();
            printf("\nEscolha o comando (h = help):  ");
            botBunmount = false;
            SD_mount = false;
        }
        if (cRxedChar == 'c') // Lista diretórios e os arquivos se pressionar 'c'
        {
            printf("\nListagem de arquivos no cartão SD.\n");
            strcpy(sd_status, "Listando");
            run_ls();
            printf("\nListagem concluída.\n");
            printf("\nEscolha o comando (h = help):  ");
        }
        if (cRxedChar == 'd') // Exibe o conteúdo do arquivo se pressionar 'd'
        {
            read_file(filename);
            printf("Escolha o comando (h = help):  ");
        }
        if (cRxedChar == 'e') // Obtém o espaço livre no SD card se pressionar 'e'
        {
            printf("\nObtendo espaço livre no SD.\n\n");
            strcpy(sd_status, "Obt. espaco");
            run_getfree();
            printf("\nEspaço livre obtido.\n");
            printf("\nEscolha o comando (h = help):  ");
        }
        if (cRxedChar == 'f' || botA) // Captura dados do sensor e salva no arquivo se pressionar 'f'
        {
            gpio_put(LED_VERDE, 0);
            gpio_put(LED_VERMELHO, 0);
            gpio_put(LED_AZUL, 1);
            strcpy(sd_status, "Lendo dados");
            tocar_buzzer(600, 300);
            capture_sensor_data_and_save();
            gpio_put(LED_VERDE, 1);
            gpio_put(LED_AZUL, 0);
            printf("\nEscolha o comando (h = help):  ");
            botA = false;
        }
        if (cRxedChar == 'g') // Formata o SD card se pressionar 'g'
        {
            printf("\nProcesso de formatação do SD iniciado. Aguarde...\n");
            strcpy(sd_status, "Formatando");
            gpio_put(LED_VERDE, 1);
            gpio_put(LED_VERMELHO, 1);
            gpio_put(LED_AZUL, 1);
            tocar_buzzer(600, 300);
            sleep_ms(200);
            run_format();
            tocar_buzzer(800, 250);
            printf("\nFormatação concluída.\n\n");
            printf("\nEscolha o comando (h = help):  ");
        }
        if (cRxedChar == 'h') // Exibe os comandos disponíveis se pressionar 'h'
        {
            run_help();
        }
        if(error_presence){   // Alerta em caso de algum erro ser detectado
            strcpy(sd_status, "Erro");
            gpio_put(LED_VERDE, 0);
            gpio_put(LED_VERMELHO, 1);
            gpio_put(LED_AZUL, 1);
            sleep_ms(500);
            gpio_put(LED_VERDE, 0);
            gpio_put(LED_VERMELHO, 0);
            gpio_put(LED_AZUL, 0);
            sleep_ms(500);
        }

        //  Atualiza o conteúdo do display com animações
        ssd1306_fill(&ssd, !cor);                           // Limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);       // Desenha um retângulo
        ssd1306_line(&ssd, 3, 32, 123, 32, cor);            // Desenha uma linha
        ssd1306_draw_string(&ssd, "Sensor MPU6050", 8, 8);  // Desenha uma string
        ssd1306_draw_string(&ssd, "Status ativo", 12, 20);   // Desenha uma string
        ssd1306_draw_string(&ssd, sd_status, 14, 46);        // Desenha uma string
        ssd1306_send_data(&ssd);                            // Atualiza o display
        
        sleep_ms(800);
        gpio_put(LED_VERDE, 0);
        gpio_put(LED_VERMELHO, 0);
        gpio_put(LED_AZUL, 0);
    }
    return 0;
}