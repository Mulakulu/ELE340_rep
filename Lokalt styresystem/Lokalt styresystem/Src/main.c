//
// Hovudprogram
// Fil: main.c
// m.t.150715

// Prosjektet saman med eit passande python-skript, gir ein dataloggar for tre 16-bits
// akselerasjonsdata med tidsreferanse
//---------------------------------------

//---------------------------------------
// Inklusjonar og definisjonar
//---------------------------------------
#include <cmsis_boot/stm32f30x.h>
#include <dekl_globale_variablar.h>

#include "stm32f3_discovery/stm32f3_discovery_lsm303dlhc.h"

//---------------------------------------
// Globale variablar
//---------------------------------------


//---------------------------------------
// Funksjonsprototypar
//---------------------------------------

void maskinvare_init(void);
void GPIO_sjekk_brytar(void);
void GPIO_sett_kompassmoenster(int8_t verdi);
void GPIO_blink(void);
void GPIO_lys_av(void);
void GPIO_lys_paa(void);
void USART2_skriv(uint8_t ch);
void USART2_send_tid8_og_data16(uint8_t tid, int16_t loggeverdi);
void USART2_send_tid8_og_data16x3(uint8_t tid, int16_t loggeverdi1, int16_t loggeverdi2, int16_t loggeverdi3);
int8_t SPI2_SendByte_Sokkel1(int8_t data);
int8_t SPI1_SendByte_gyro(int8_t data);
int8_t les_gyrotemp(void);
void vent_400nsek(void);

//---------------------------------------
// Funksjonsdeklarasjonar
//---------------------------------------

int main(void)  {
    uint16_t a;

    maskinvare_init();

	while(1) {

		if(oppdater_diodar) {  // Blir gjort kvart 200. msek, sjå fila avbrotsmetodar

			GPIO_sett_kompassmoenster(diode_moenster);
			diode_moenster = diode_moenster + 0x2;
			oppdater_diodar = 0;

		}

		if(gyldig_trykk_av_USERbrytar) { //Er brytaren trykt ned sidan sist?
                                         // Skal då laga ei ekstramelding.
		    uint8_t data0, data1;

            data0 = diode_moenster;
            data1 = data0 >> 4;

        	USART2_skriv('S');    //Viser at det er ei brytarmelding (S for svitsj)
        	USART2_skriv((uint8_t)(hex2ascii_tabell[(data1 & 0x0F)]));   // Send MS Hex-siffer av ein tidsbyte
        	USART2_skriv((uint8_t)(hex2ascii_tabell[(data0 & 0x0F)])); // Send LS Hex-siffer av ein tidsbyte

        	gyldig_trykk_av_USERbrytar = 0;

        	diode_moenster = 0; 		  // Nullstill diodem�nsteret etter eit brytartrykk
        }

	 // Her er hovudoppg�va til dette prosjektet, nemleg avlesing av 3D akselerasjonsdata og sending av dette
	 // til PC.
		if(send_maalingar_til_loggar)  {   // Viss brukaren har sett i gong logging ved � trykkja 'k' p�
			                               // tastaturet.
        	if(legg_til_meldingshovud)  {
        		USART2_skriv((uint8_t) STX); // Meldingsstart aller f�rst
        		legg_til_meldingshovud = 0;
        	}

        	if(ny_maaling) { // Det skal utf�rast ny m�ling med 100Hz samplefrekvens (ordna i fila avbrotsmetodar)

		        LSM303DLHC_Read(ACC_I2C_ADDRESS, LSM303DLHC_OUT_X_L_A, buffer, 6); // OUT_X_L_A ligg på lågaste adresse
		        																   // i XYZ-registerblokka i kretsen
			 // Sett saman akselerometerdata
                a   = (buffer[1] << 8) | buffer[0]; //Buffer 1 er MSByte i flg. databladet.
		        a_x = ((int16_t)(a)) >> 4;          //Gjer om til int der bare dei 12 teljande bitane er med
				a   = (buffer[3] << 8) | buffer[2];	//Dette gir omr�det -2048 til 2047. Oppl�ysinga er
				a_y = ((int16_t)(a)) >> 4;			// 1mg pr. LSb ved +/-2g omr�de i flg. databladet.
				a   = (buffer[5] << 8) | buffer[4];	// Verdien 1000 gir d� 1 g.
				a_z = ((int16_t)(a)) >> 4;

			 // Filtrer m�lingane
				a_xf_k = (a1*a_xf_k_1 + b1*a_x)/100; //Nedsamplingsfilter, sj� deklarasjonsfila
				a_xf_k_1 = a_xf_k;

				a_yf_k = (a1*a_yf_k_1 + b1*a_y)/100; //Nedsamplingsfilter, sj� deklarasjonsfila
				a_yf_k_1 = a_yf_k;

				a_zf_k = (a1*a_zf_k_1 + b1*a_z)/100; //Nedsamplingsfilter, sj� deklarasjonsfila
				a_zf_k_1 = a_zf_k;

			 // Send kvar tiande m�ling
				if(send_maaling) {

                	samplenr++;                      // Oppdatering av tidsreferanse

    		        USART2_send_tid8_og_data16x3(samplenr, (int16_t) a_xf_k, (int16_t) a_yf_k, (int16_t) a_zf_k);

    		        send_maaling = 0; // Sendinga er n� utf�rt.
                }

    		    ny_maaling = 0; // M�linga er n� utf�rt
			 }

        }
		else if(!send_maalingar_til_loggar) { // Her har brukaren trykt ein 's' på tastaturet, eller
											  // logginga har aldri blitt starta.
			if(legg_til_meldingshale)  {
			     USART2_skriv((uint8_t) ETX); // Meldingshale for å avslutta loggemeldingane etter stopping.
			     legg_til_meldingshale = 0;
			}
		}

     }
}



