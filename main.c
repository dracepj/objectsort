
#include "MKL25Z4.h"
#include "user_def.h"
#include "LEDs.h"

//Global Variables
const char halfsteps[ ] = {0x08, 0x0C, 0x04, 0x06, 0x02, 0x03 , 0x01, 0x09};
int i = 0; 
char view; 
int Threshold[NUM_RANGE_STEPS] = {1100, 650, 400, 270, 200, 0};

const int Colors[NUM_RANGE_STEPS][3] = {{ 1, 1, 1}, // white
				        { 1, 0, 1}, // magenta
					{ 1, 0, 0}, // red
					{ 1, 1, 0}, // yellow
					{ 0, 0, 1}, // blue
					{ 0, 1, 0}};// green
																	 

void Init_ADC(void) {
	
	SIM->SCGC6 |= (1UL << SIM_SCGC6_ADC0_SHIFT); 
	ADC0->CFG1 = 0x9c; // 16 bit precision
	ADC0->SC2 = 0;
}

//switch on and off IR_LED
void Control_IR_LED(unsigned int led_on) {
	if (led_on) {
			PTB->PCOR = MASK(IR_LED_POS);
	} else {
			PTB->PSOR = MASK(IR_LED_POS); 
	}
}	

//initilize IR_LED by using the PCR register; identify the pin number
//on the PORTB to connect it to PORTB 
void Init_IR_LED(void) {
	PORTB->PCR[IR_LED_POS] &= ~PORT_PCR_MUX_MASK;          
	PORTB->PCR[IR_LED_POS] |= PORT_PCR_MUX(1);          
	PTB->PDDR |= MASK(IR_LED_POS);
	
	// start off with IR LED turned off
	Control_IR_LED(0);
}

unsigned Measure_IR(void) {
	volatile unsigned res=0;
	
	ADC0->SC1[0] = IR_PHOTOTRANSISTOR_CHANNEL; // start conversion on channel 0
	
	while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK));
	res = ADC0->R[0];
	// complement result since voltage falls with increasing IR level
	// but we want result to rise with increasing IR level
	return 0xffff-res;
}

void Display_Range(int b) {
	unsigned i;
	
	for (i=0; i<NUM_RANGE_STEPS-1; i++) {
		if (b > Threshold[i])
			break;
	}
	Control_RGB_LEDs(Colors[i][RED], Colors[i][GREEN], Colors[i][BLUE]);
}

void IR_delay(int n) {
	int i, j;
    for(i = 0 ; i < n; i++)
        for(j = 0; j < 500; j++)
            {}  /* do nothing */
}

void runForward(){
	PTC->PDOR &= ~(0x0F << 3);	/* clear motor enable pins */
		PTC->PDOR |= halfsteps[i++ & 7] << 3;           /* set the motor enable pins */
		delay(2);
	
}

void runBackward(){
		PTC->PDOR &= ~(0x0F << 3); /* clear motor enable pins */
		PTC->PDOR |= halfsteps[i-- & 7] << 3;  /* set the motor enable pins */
		delay(2);
}


void delay(int n){
	/* delay n milliseconds (41.94MHz CPU clock) */
    int i, j;
    for(i = 0 ; i < n; i++)
        for(j = 0; j < 1000; j++)
            {}  /* do nothing */
}

void Init_motor_ports(){
	 /* enable PTC 6-3 as output */
    SIM->SCGC5 |= 0x800;        /* enable clock to Port C */
		PORTC->PCR[1] = 0x100; 
    PORTC->PCR[3] = 0x100;      /* make PTC3 pin as GPIO */
    PORTC->PCR[4] = 0x100;      /* make PTC4 pin as GPIO */
    PORTC->PCR[5] = 0x100;      /* make PTC5 pin as GPIO */
    PORTC->PCR[6] = 0x100;      /* make PTC6 pin as GPIO */
    PTC->PDDR |= 0xf8;          /* make PTC6-3 as output pin */
    
    /* enable PTA 5, 12 as output */
    SIM->SCGC5 |= 0x200;        /* enable clock to Port A */
    PORTA->PCR[5] = 0x100;      /* make PTA5 pin as GPIO */
    PORTA->PCR[12] = 0x100;     /* make PTA12 pin as GPIO */
    PTA->PDDR |= 1 << 5 | 1 << 12;  /* make PTA5, 12 as output pin */
    PTA->PDOR |= 1 << 5 | 1 << 12;  /* make PTA5, 12 high to enable motor drive */
	
}




void Init_RGB_LEDs() {
	// Enable clock to ports B and D
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTD_MASK;;
	
	// Make 3 pins GPIO
	PORTB->PCR[RED_LED_POS] &= ~PORT_PCR_MUX_MASK;          
	PORTB->PCR[RED_LED_POS] |= PORT_PCR_MUX(1);          
	PORTB->PCR[GREEN_LED_POS] &= ~PORT_PCR_MUX_MASK;          
	PORTB->PCR[GREEN_LED_POS] |= PORT_PCR_MUX(1);          
	PORTD->PCR[BLUE_LED_POS] &= ~PORT_PCR_MUX_MASK;          
	PORTD->PCR[BLUE_LED_POS] |= PORT_PCR_MUX(1);          
	
	// Set ports to outputs
	PTB->PDDR |= MASK(RED_LED_POS) | MASK(GREEN_LED_POS);
	PTD->PDDR |= MASK(BLUE_LED_POS);
}

void Control_RGB_LEDs(unsigned int red_on, unsigned int green_on, unsigned int blue_on) {
	if (red_on) {
			PTB->PCOR = MASK(RED_LED_POS);
	} else {
			PTB->PSOR = MASK(RED_LED_POS); 
	}
	if (green_on) {
			PTB->PCOR = MASK(GREEN_LED_POS);
	}	else {
			PTB->PSOR = MASK(GREEN_LED_POS); 
	} 
	if (blue_on) {
			PTD->PCOR = MASK(BLUE_LED_POS);
	}	else {
			PTD->PSOR = MASK(BLUE_LED_POS); 
	}
}	





unsigned controlIR(void){
	unsigned on_brightness=0, off_brightness=0;
	static int avg_diff;
	int diff;
	unsigned n;
	
	while(1){
		diff = 0; 
		for (n=0; n<10; n++) {
			// measure IR level with IRLED off
			Control_IR_LED(0);
			IR_delay(1);
			off_brightness = Measure_IR();
		
			// measure IR level with IRLED on
			Control_IR_LED(1);
			IR_delay(1);
			on_brightness = Measure_IR();

			// calculate difference
			diff += on_brightness - off_brightness; 
		}
		avg_diff = diff/10;
		return avg_diff; 
		
	}
	
}

int main(void)
{
//	unsigned on_brightness=0, off_brightness=0;
//	static int avg_diff;
//	int diff;
//	unsigned n;
//	
	Init_motor_ports();
	Init_ADC();
	Init_RGB_LEDs();
	Init_IR_LED();
	Control_RGB_LEDs(0, 0, 0);
	//  controlIR();
	int boundary = 0;	
	
	
	boundary = 0.8 * (controlIR() + controlIR())/2; //ir threshhold
	static int avg_diff = 0;
					//count how long backward
	int boundaryResetCount = 0;  
	int j = 0;
	
	while (1) {
		if(boundaryResetCount == 90){	//every 2-iters capture new environment lighting data
			boundary = 0.7 * (controlIR() + controlIR())/2;
			boundaryResetCount = 0;
		}
		avg_diff = controlIR();
		boundaryResetCount++;
		if(avg_diff <= boundary){		//if ir > boundary,
				boundaryResetCount = 0; 
				boundary = 1000000; 
				runForward();
				j++; 
				if(j > 3000){
					boundaryResetCount = 90;
					boundary = 0;
					j=0; 
				}
		}else{
				runBackward();	
		}
	}
}


