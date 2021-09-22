#include "mbed.h"
#include <stdlib.h>


/**
 * @brief Estos son los estados de la MEF (Máquina de Estados Finitos), para comprobar el estado del boton.
 * El botón está configurado como PullUp, establece un valor lógico 1 cuando no se presiona
 * y cambia a valor lógico 0 cuando es presionado.
 */
typedef enum{
    BUTTON_DOWN,        //0
    BUTTON_UP,          //1
    BUTTON_FALLING,     //2
    BUTTON_RISING       //3
}_eButtonState;

/**
 * @brief Estructura que define a los pulsadores.
 * Posicion (pos): Como un entero, para saber que boton es el que se presionó
 * Estado: Como un entero, para saber si el boton fue presionado.
 * TimeDown: definido como un entero, que guarda el tiempo que el boton estuvo presionado, para no provocar errores.
 * TimeDiff: Es la resta entre el valor del tiempo del programa, y el valor de TimeDown, para saber cuando entrar al juego.
 */
typedef struct{
    int pos;
    uint8_t estado;
    int32_t timeDown;
    int32_t timeDiff;
}_sTeclas;

#define TRUE    1
#define FALSE    0

#define TIMEMAX    2001//1501
#define BASETIME   5000

/**
 * @brief Tiempo mínimo de presionado de un boton para iniciar el juego
 * 
 */
#define TIMETOSTART    1000

/**
 * @brief Intervalo entre presiones de botones para "filtrar" el ruido.
 * 
 */
#define INTERVAL    40

/**
 * @brief Tiempo en el que el LED que se prendió cambia de estado.
 * 
 */
#define ESTADO    1000

/**
 * @brief Intervalo donde cambian de estado los cuatro led por causa de ganarle al topo 
 * 
 */
#define CAMBIO    500

/**
 * @brief Cantidad de botones
 * 
 */
#define NROBOTONES    4

/**
 * @brief Cantidad de Leds
 * 
 */
#define MAXLED    4

/**
 * @brief Valor del acumulador para el parpadeo
 * 
 */
#define PARPADEO 6

/**
 * @brief Estado inicial de la Maquina de Estado del juego.
 *  Este va a cambiar, cuando el usuario presione por más de 1 segundo un boton.
 */
#define STANDBY    0

/**
 * @brief Segundo estado de la Maquina de Estado del juego.
 *  Se controla que se haya presionado un solo botón. 
 */
#define KEYS    1

/**
 * @brief Tercer estado de la Maquina de Estado del juego.
 * Arranca el juego
 * Enciende un LED aleatorio
 * A ese LED aleatorio, se le asigna un tiempo aleatorio para "pegarle" (tocar el boton correspondiente)
 */
#define GAMEMOLE    2

/**
 * @brief Cuarto estado de la Maquina de Estado del juego.
 * Comprueba si el boton presionado corresponde al LED encendido
 * Si acierta, pasa al estado WIN
 * Si falla, pasa al estado MOLECELEBRATES
 */
#define COMP    3

/**
 * @brief Quinto estado de la Maquina de Estado del juego.
 * El usuario GANÓ
 * Los cuatro LED parpadean 3 veces.
 * Vuelve al estado STANBY
 */
#define WIN   4

/**
 * @brief Ultimo estado de la Maquina de Estado del juego. 
 * El usuario PERDIÓ
 * "El topo festeja"
 * El LED que se prendió parpadea 3 veces y vuelve al estado STANDBY
 */
#define MOLEWIN   5


/**
 * @brief Inicializa la MEF
 * Le dá un estado inicial a indice
 * @param indice Este parametro recibe el estado de la MEF
 */
void startMef(uint8_t indice);

/**
 * @brief Máquina de Estados Finitos(MEF)
 * Actualiza el estado del botón cada vez que se invoca.
 * 
 * @param indice Este parámetro le pasa a la MEF el estado del botón.
 */
void actuallizaMef(uint8_t indice );

/**
 * @brief Función para cambiar el estado del LED cada vez que sea llamada.
 * 
 */
void togleLed(uint8_t indice);




/**
 * @brief Dato del tipo Enum para asignar los estados de la MEF
 * 
 */
_eButtonState myButton;

_sTeclas ourButton[NROBOTONES];

uint16_t mask[]={0x0001,0x0002,0x0004,0x0008};//!<Mascara con las posiciones de los botones o leds

BusIn botones(PB_6,PB_7,PB_8,PB_9);
BusOut leds(PB_12,PB_13,PB_14,PB_15);


Timer miTimer; //!< Timer para hacer la espera de 40 milisegundos

int tiempoMs=0; //!< variable donde voy a almacenar el tiempo del timmer una vez cumplido

uint8_t estado=STANDBY;

unsigned int ultimo=0;  //!< variable para el control del heartbeats. guarda un valor de tiempo en ms

uint16_t ledAuxRandom=0;
uint16_t ledOn; //Guarda el led que se activó
uint16_t posBoton;//guarda el pulsador que se activo

int main()
{
    miTimer.start();
    int tiempoRandom=0;
    int ledAuxRandomTime=0;
    int ledAuxJuegoStart=0;
    uint8_t indiceAux=0;
    uint8_t control=FALSE;
    int check=0;
    int acumTime=0;
    uint8_t acum=1;
 
    for(uint8_t indice=0; indice<NROBOTONES;indice++){
        startMef(indice);
        ourButton[indice].pos = indice;
    }

    while(TRUE)
    {
        if((miTimer.read_ms()-ultimo)>ESTADO){
           ultimo=miTimer.read_ms();
        }  

        switch(estado){
            case STANDBY:
                if ((miTimer.read_ms()-tiempoMs)>INTERVAL){
                    tiempoMs=miTimer.read_ms();
                    for(uint8_t indice=0; indice<NROBOTONES;indice++){
                        actuallizaMef(indice);
                        if(ourButton[indice].timeDiff >= TIMETOSTART){
                            srand(miTimer.read_us());
                            estado=KEYS;   
                        }
                    }
                }
            break;
            case KEYS:
                for( indiceAux=0; indiceAux<NROBOTONES;indiceAux++){
                    actuallizaMef(indiceAux);
                    if(ourButton[indiceAux].estado!=BUTTON_UP){
                        break;
                    }
                        
                }
                if(indiceAux==NROBOTONES){
                    estado=GAMEMOLE;
                    leds=15;
                    ledAuxJuegoStart=miTimer.read_ms();
                }
            break;
            case GAMEMOLE:

                if(leds==0){
                    tiempoRandom=miTimer.read_ms();
                    ledAuxRandom = rand() % (MAXLED);   //selecciona uno de los 4 led para encender
                    ledAuxRandomTime = ((rand() % TIMEMAX)+BASETIME); //Determina cuanto tiempo estara encendido el led
                    //leds=mask[ledAuxRandom];
                    togleLed(ledAuxRandom);
                    control=0;
                }else{
                    if((miTimer.read_ms() - ledAuxJuegoStart)> TIMETOSTART) {
                        if(leds==15){
                            ledAuxJuegoStart=miTimer.read_ms();
                            leds=0;
                        }
                    }
                }
                
                if (((miTimer.read_ms()-ledAuxJuegoStart)>ledAuxRandomTime)&&(control)){
                    estado=MOLEWIN;
                }
                
                if((leds!=0)&&(leds!=15)){
                    if((miTimer.read_ms()-check)>INTERVAL){
                        check=miTimer.read_ms();
                        for(uint8_t indice=0; indice<NROBOTONES;indice++){
                            actuallizaMef(indice);
                            if(ourButton[indice].estado==BUTTON_DOWN){
                                estado=COMP;
                                posBoton=ourButton[indice].pos;
                                ledOn=ledAuxRandom;
                            }
                        }                            
                    }
                }

            break;
            case COMP:
                if(mask[ledOn]==mask[posBoton]){
                    estado=WIN;  
                }else{
                    estado=MOLEWIN;
                }
            break;
            case MOLEWIN:
                if((miTimer.read_ms()-acumTime)>CAMBIO){
                    acumTime = miTimer.read_ms();
                    
                    if((acum%2)==0){
                        leds=0;
                    }else{
                        togleLed(ledAuxRandom);
                    }
                    if(acum==PARPADEO){
                        estado=STANDBY;
                        acum=0;
                    }
                    acum++;
                }                
            break;
            case WIN:
                if((miTimer.read_ms()-acumTime)>CAMBIO){
                    acumTime = miTimer.read_ms();
                    
                    if((acum%2)==0){
                        leds=0;
                    }else{
                        leds=15;
                    }
                    if(acum==PARPADEO){
                        estado=STANDBY;
                        acum=0;
                    }
                    acum++;
                }
                            
            break;
            default:
                estado=STANDBY;
        }
    }
    return 0;
}



void startMef(uint8_t indice){
   ourButton[indice].estado=BUTTON_UP;
}

void actuallizaMef(uint8_t indice){

    switch (ourButton[indice].estado)
    {
    case BUTTON_DOWN:
        if(botones.read() & mask[indice] )
           ourButton[indice].estado=BUTTON_RISING;
    
    break;
    case BUTTON_UP:
        if(!(botones.read() & mask[indice]))
            ourButton[indice].estado=BUTTON_FALLING;
    
    break;
    case BUTTON_FALLING:
        if(!(botones.read() & mask[indice]))
        {
            ourButton[indice].timeDown=miTimer.read_ms();
            ourButton[indice].estado=BUTTON_DOWN;
            //Flanco de bajada
        }
        else
            ourButton[indice].estado=BUTTON_UP;    

    break;
    case BUTTON_RISING:
        if(botones.read() & mask[indice]){
            ourButton[indice].estado=BUTTON_UP;
            //Flanco de Subida
            ourButton[indice].timeDiff=miTimer.read_ms()-ourButton[indice].timeDown;
        }

        else
            ourButton[indice].estado=BUTTON_DOWN;
    
    break;
    
    default:
    startMef(indice);
        break;
    }
}

void togleLed(uint8_t indice){
   leds=mask[indice];
}