/* Bibliotecas */
#include "mbed.h"
#include "CANMsg.h"

#define  CAN_ID_TX 0x100
#define BLINKING_RATE 100ms



CAN can(PD_0, PD_1, 125000);
CANMsg txMsg;



/* Pinagem */
#define sensor_apps1  PA_5                           // Sinal de entrada do sensor do apps 1
#define sensor_apps2  PA_6                           // Sinal de entrada do sensor do apps 2
#define sensor_bse    PF_4                           // Sinal de entrada do sensor do bse
#define pino_inversor PA_4                           // Sinal de saída para o inversor
#define pino_teste PA_3


/* Adicionar o timer de maneira legível */
#define timer_read_ms(x)    chrono::duration_cast<chrono::milliseconds>((x).elapsed_time()).count()


/* Estrutura de dados */
typedef enum {FALHA, TORQUE} state;


/* Classes API */
AnalogOut inversor_envio(pino_inversor);
AnalogIn apps1_aq(sensor_apps1);
AnalogIn apps2_aq(sensor_apps2);
AnalogIn bse_aq(sensor_bse);
Timer tempo_erro;

/* Flag da CAN*/

volatile bool cantx_flag = false;

/* Variaveis CAN*/

uint16_t rpm;
uint16_t pwm;
uint16_t corrente;
uint16_t RPM_inter;
uint16_t RPM_inter2;

// Variaveis para a medicao de falha
bool  flag_plausibility = 0;
bool  flag_falha_sensores = 0;
float inversor;
float apps1;
float apps2;
float bse;



//Declaraveis
float max_apps1 = 3.33;                              // Sinal máximo do sensor 1 do APPS
float min_apps1 = 0.37;                              // Sinal minimo do sensor 1 do APPS
float max_apps2 = 3.33;                              // Sinal máximo do sensor 2 do APPS
float min_apps2 = 0.37;                              // Sinal minimo do sensor 2 do APPS
float max_bse = 2.93;                                // Sinal maximo do sensor do BSE
float min_bse = 0.73;                                // Sinal minimo do sensor do BSE

//Maquina de estados
state estado = TORQUE;                               // Variavel para o controle da maq de estados


/* Declaracao das funcoes */
void convertevalores();
void verifica_falha_sensores();
void freio_plausibility_check();
uint16_t to_pwm(float signal_apps_porcentagem);


/* Interrupt CAN*/
void cantx_isr();
/* Função Main */
int main() {


    //attacheamento da função da CAN

    can.attach(&cantx_isr, CAN::TxIrq);

    //Setando as tensoes de referencia para o ADC
    
    apps1_aq.set_reference_voltage(3.3);
    apps2_aq.set_reference_voltage(3.3);
    bse_aq.set_reference_voltage(3.3);

    while (true) {
        cantx_flag = true;
        convertevalores();
        verifica_falha_sensores();
        freio_plausibility_check();
        /* Maquina de estados */
        if(cantx_flag){
            cantx_flag = false;
            txMsg.clear();
            txMsg.len = 8;
 
        
        switch (estado) {

            case FALHA:
                pwm = 0;
                inversor_envio.write(0);
                printf("Dentro da FALHA, mandando 0V \n");
                break;

            case TORQUE:
                pwm = to_pwm(apps1);
                inversor_envio.write(pwm);
                //printf("Valor percentual do apps1: %f \n", inversor_envio.read_volt());
                printf("Enviando %f de tensao \n", inversor_envio.read()*3.3);
                break;
        }
            txMsg.data[0] = rpm & 0xFF;
            txMsg.data[1] = rpm >> 8;
            txMsg.data[2] = 0;
            txMsg.data[3] = pwm & 0xFF;
            txMsg.data[4] = pwm >> 8;
            txMsg.data[5] = corrente & 0xFF;
            txMsg.data[6] = corrente >> 8;
            txMsg.data[7] = 0b00000000;
            
            can.write(txMsg);

            printf("\r\n\t PWM LOW = %d", txMsg.data[3]);
            printf("\r\n\t PWM HIGH = %d", txMsg.data[4]);
            printf("\r\n\t PMW = %d", pwm);
        }
       
    }
}


/* Corpo das funcoes */
// Funcao responsavel para mapear os valores dos 2 sensores do apps
void convertevalores () {
    apps1 = (apps1_aq.read_voltage() - min_apps1) / (max_apps1 - min_apps1);
    apps2 = (apps2_aq.read_voltage() - min_apps2) / (max_apps2 - min_apps2);
    bse =   (bse_aq.read_voltage() - min_bse)     / (max_bse - min_bse);
}


//Funcao responsavel por detectar se ha algum erro das medicoes dos 3 sensores e disparidade dos 2 apps
void verifica_falha_sensores() {

    //printf("Apps1 = %f e Apps2 = %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
    if ( (apps1 > (1.1*apps2)) || (apps1 < (0.9*apps2)) ) {
        printf("Erro de 10 porcento entre os sensores, %f e %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
        //estado = CHECK; 
        flag_falha_sensores = 1;
    }
    else {
        if ( (apps1_aq.read_voltage() > max_apps1) ) {
            printf("Apps1 acima da escala maxima, %f e %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
            //estado = CHECK;
            flag_falha_sensores = 1;
        }
        else {
            if( (apps2_aq.read_voltage() > max_apps2) ) {
                printf("Apps2 acima da escala maxima, %f e %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
                //estado = CHECK;
                flag_falha_sensores = 1;
            }
            else {
                if  (apps1_aq.read_voltage() < min_apps1 ) {
                    printf ("Apps1 abixo da escala minima, %f e %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
                    //estado = CHECK;
                    flag_falha_sensores = 1;
                }
                else {
                    if (apps2_aq.read_voltage() < min_apps2) {
                        printf("Apps2 abaixo da escala minima, %f e %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
                        //estado = CHECK;
                        flag_falha_sensores = 1;
                    }
                    else {
                        if(bse_aq.read_voltage() > max_bse) {
                            printf("BSE acima da escala maxima, %f \n", bse_aq.read_voltage());
                            //estado = CHECK;
                            flag_falha_sensores = 1;
                        }
                        else {
                            if(bse_aq.read_voltage() < min_bse) {
                                printf("BSE abaixo da escala minima, %f \n", bse_aq.read_voltage());
                                //estado = CHECK;
                                flag_falha_sensores = 1;
                            }
                            else {
                                estado = TORQUE;
                                flag_falha_sensores = 0;
                            }
                        }
                    }
                }
            }
        }
    }

    if(flag_falha_sensores == 1) {
        tempo_erro.start();
        if(timer_read_ms(tempo_erro) > 100) {
            printf("%llu \n", timer_read_ms(tempo_erro));
            estado = FALHA;
            tempo_erro.stop(); 
            tempo_erro.reset();
            }
    } else {
        tempo_erro.stop(); 
        tempo_erro.reset();
    }
}


//Funcao responsavel por identificar se ha presença de pressionamento de freio e pedal simultaneos
void freio_plausibility_check() {
    if((apps1 > 0.25) && (bse > 0.1) && (flag_plausibility == 0)) {
        estado = FALHA;
        flag_plausibility = 1;
        printf("Erro plausibility check \n");
    } else {
        if((flag_plausibility == 1) && (apps1 < 0.05)) {
            printf("FALHA DE PLAUSIBILIDADE \n");
            estado = TORQUE;
            flag_plausibility = 0;
        }
    }
}
// Função responsável por converter o sinal em porcentagem para PWM inteiro.
uint16_t to_pwm(float signal_apps_porcentagem){

    return signal_apps_porcentagem * 65535;

}

void cantx_isr(){
    cantx_flag = true;
};
