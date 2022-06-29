/* Bibliotecas */
#include "mbed.h"


/* Pinagem */
#define sensor_apps1  PA_5                          // Sinal de entrada do sensor do apps 1
#define sensor_apps2  PA_6                          // Sinal de entrada do sensor do apps 2
#define sensor_bse    PA_0                          // Sinal de entrada do sensor do bse
#define pino_inversor PA_4                          // Sinal de saída para o inversor


/* Adicionar o timer de maneira legível */
#define timer_read_ms(x)    chrono::duration_cast<chrono::milliseconds>((x).elapsed_time()).count()


/* Estrutura de dados */
typedef enum {CHECK, FALHA, TORQUE} state;


/* Classes API */
AnalogOut inversor_envio(pino_inversor);
AnalogIn apps1_aq(sensor_apps1);
AnalogIn apps2_aq(sensor_apps2);
AnalogIn bse_aq(sensor_bse);
Timer tempo_erro;
//InterruptIn bse_aq(sensor_bse);


/* Variaveis */
// Variaveis para a medicao de falha
bool  flag_falha;
float inversor;
float apps1;
float apps2;
float bse;

//Declaraveis
float max_apps1 = 4.5;                              // Sinal máximo do sensor 1 do APPS
float min_apps1 = 0.5;                              // Sinal minimo do sensor 1 do APPS
float max_apps2 = 4.5;                              // Sinal máximo do sensor 2 do APPS
float min_apps2 = 0.5;                              // Sinal minimo do sensor 2 do APPS
float max_bse = 3.0;                                // Sinal maximo do sensor do BSE
float min_bse = 1.3;                                // Sinal minimo do sensor do BSE

//Maquina de estados
state estado;                                       // Variavel para o controle da maq de estados


/* Declaracao das funcoes */
void convertevalores();
void verifica_falha_sensores();
void freio_plausibility_check();


/* Função Main */
int main() {

    //Setando as tensoes de referencia para o ADC
    apps1_aq.set_reference_voltage(3.3);
    apps2_aq.set_reference_voltage(3.3);

    while (true) {
        convertevalores();
        verifica_falha_sensores();
        freio_plausibility_check();

        /* Maquina de estados */
        switch (estado) {

            case CHECK:
                tempo_erro.start();
                if(timer_read_ms(tempo_erro) > 100) {
                    estado = FALHA;
                    tempo_erro.stop(); 
                    tempo_erro.reset();
                } else {
                    estado = TORQUE;
                }
                break;

            case FALHA:
                inversor_envio.write(0);
                printf("Dentro da FALHA, mandando 0V \n");
                break;

            case TORQUE:
                inversor_envio.write(apps1);
                printf("Enviando %f de tensao \n", inversor_envio.read()*3.3);
                break;
        }
    }
}


/* Corpo das funcoes */
// Funcao responsavel para mapear os valores dos 2 sensores do apps
void convertevalores () {
    apps1 = (apps1_aq.read_voltage() - min_apps1) / (max_apps1 - min_apps1);
    apps2 = (apps2_aq.read_voltage() - min_apps2) / (max_apps2 - min_apps2);
}


//Funcao responsavel por detectar se ha algum erro das medicoes dos 3 sensores e disparidade dos 2 apps
void verifica_falha_sensores() {

    //printf("Apps1 = %f e Apps2 = %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
    if ( (apps1 > (1.1*apps2)) || (apps1 < (0.9*apps2)) ) {
        printf("Erro de 10 porcento entre os sensores, %f e %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
        estado = CHECK; 
    }
    else {
        if ( (apps1_aq.read_voltage() > max_apps1) ) {
            printf("Apps1 acima da escala maxima, %f e %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
            estado = CHECK;
        }
        else {
            if( (apps2_aq.read_voltage() > max_apps2) ) {
                printf("Apps2 acima da escala maxima, %f e %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
                estado = CHECK;
            }
            else {
                if  (apps1_aq.read_voltage() < min_apps1 ) {
                    printf ("Apps1 abixo da escala minima, %f e %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
                    estado = CHECK;
                }
                else {
                    if (apps2_aq.read_voltage() < min_apps2) {
                        printf("Apps2 abaixo da escala minima, %f e %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
                        estado = CHECK;
                    }
                    else {
                        if(bse_aq.read_voltage() > max_bse) {
                            printf("BSE acima da escala maxima, %f \n", bse_aq.read_voltage());
                            estado = CHECK;
                        }
                        else {
                            if(bse_aq.read_voltage() < min_bse) {
                                printf("BSE abaixo da escala minima, %f \n", bse_aq.read_voltage());
                                estado = CHECK;
                            }
                            else {
                                estado = TORQUE;
                            }
                        }
                    }
                }
            }
        }
    }
}


//Funcao responsavel por identificar se ha presença de pressionamento de freio e pedal simultaneos
void freio_plausibility_check() {
    if((apps1 > 0.25) && (bse > 0.1)) {
        estado = FALHA;
        flag_falha = 0;
    } else {
        if(flag_falha == 0 && apps1 < 0.05) {
            estado = TORQUE;
            flag_falha = 1;
        }
    }
}