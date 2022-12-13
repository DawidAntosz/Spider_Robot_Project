#include <FlexiTimer2.h>
#include <Servo.h>   
#include <Wire.h>

#define resistor        1000
#define ledpin          A3
#define seriesresistor  10000
#define seriesresistor  10000 

Servo servo[4][3];
const int servo_pin[4][3] = { {2, 3, 4}, {5, 6, 7}, {8, 9, 10}, {11, 12, 13} };

int wart        ;
int przod  = 14 ;
int tyl    = 15 ;
int prawo  = 16 ;
int lewo   = 17 ;
int stoj   = 18 ;
int siedz  = 19 ;
int power  = 0  ;
int zmiana = 1  ;
int flagowa_przeszkoda = 0 ;
int mode_lewo_prawo    = 0 ;


volatile float polozenie_teraz   [4][3]                            ;   
volatile float polozenie_wlasciwe[4][3]                            ; 
float speede                     [4][3]                            ;   
float predkosc_ruchu                                               ;     
float mnoznik_predkosci           = 1                              ; 
float predkosc_obrotu_miejscowego = 4                              ;   
float predkosc_ruchu1             = 8                              ;
float predkosc_ruchu2             = 3                              ;
float LightResistant                                               ;
float logR2, T, Tc                                                 ;
float resistance                                                   ;
float c1 = 1.00924e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07 ;  
float odczyt                                                       ;

volatile int licznik                                               ; 

const float length_a    =  55                                      ;
const float length_b    =  77.5                                    ;
const float length_c    =  27.5                                    ;
const float length_side =  71                                      ;
const float z_absolute  = -28                                      ;

const float z_default = -50, z_up     = -30, z_boot = z_absolute   ;
const float x_default =  62, x_offset = 0                          ;
const float y_start   =  0 ,  y_step  = 40                         ;

const float predkosc_siedz_wstan = 1                               ;      
const float USTAW  = 255                                           ;
const float pi     = 3.1415926                                     ;

const float temp_a = sqrt(pow(2 * x_default + length_side, 2) + pow(y_step, 2))  ;
const float temp_b = 2 * (y_start + y_step) + length_side                        ;
const float temp_c = sqrt(pow(2 * x_default + length_side, 2) + pow(2 * y_start + y_step + length_side, 2))                                                        ;
const float temp_alpha = acos((pow(temp_a, 2) + pow(temp_b, 2) - pow(temp_c, 2)) / 2 / temp_a / temp_b)                                                             ;

const float turn_x1 = (temp_a - length_side) / 2                       ;
const float turn_y1 = y_start + y_step / 2                             ;
const float turn_x0 = turn_x1 - temp_b * cos(temp_alpha)               ;
const float turn_y0 = temp_b * sin(temp_alpha) - turn_y1 - length_side ;


byte data = 0 ;

//////////////////////

void setup()
{
  Serial.begin(9600)                ;
  Wire.begin(8)                     ;
  Wire.onRequest(requestEvent)      ;
  pinMode (ledpin ,   OUTPUT      ) ;
  pinMode (przod  ,   INPUT_PULLUP) ;
  pinMode (tyl    ,   INPUT_PULLUP) ;
  pinMode (prawo  ,   INPUT_PULLUP) ;
  pinMode (lewo   ,   INPUT_PULLUP) ;
  pinMode (stoj   ,   INPUT_PULLUP) ;
  pinMode (siedz  ,   INPUT_PULLUP) ;

  ustawienie(0, x_default - x_offset, y_start + y_step, z_boot) ;
  ustawienie(1, x_default - x_offset, y_start + y_step, z_boot) ;
  ustawienie(2, x_default + x_offset, y_start, z_boot)          ;
  ustawienie(3, x_default + x_offset, y_start, z_boot)          ;
 
  for (int i = 0; i < 4; i++)
    {
         for (int j = 0; j < 3; j++)
           {
               polozenie_teraz[i][j] = polozenie_wlasciwe[i][j]    ;
           }
    }
  
  FlexiTimer2::set(20, Serwa)                                ;
  FlexiTimer2::start()                                       ;
  zalacz_serwa()                                             ;
  
  while (!Serial)
  {
     ;
  }
   
}



void loop()
{
  int tmp_turn, tmp_leg, tmp_body                                                ;
   
   if (is_stand())
    {
        tmp_turn         =    predkosc_obrotu_miejscowego                        ;
        tmp_leg          =    predkosc_ruchu1                                    ;
        tmp_body         =    predkosc_ruchu2                                    ;
        predkosc_obrotu_miejscowego  =    predkosc_ruchu1 = predkosc_ruchu2 = 20 ;
        if (flagowa_przeszkoda < 3)
          {
             krok_w_tyl (1)                                     ;
             flagowa_przeszkoda++                               ;
          }
        else
          {
             if (mode_lewo_prawo)              
                 skrec_w_prawo (1)                              ;
              
             else              
                 skrec_w_lewo (1)                               ;               
                 mode_lewo_prawo = 1 - mode_lewo_prawo          ;
                 flagowa_przeszkoda   = 0                       ;
                                  
          }         
        predkosc_obrotu_miejscowego  =   tmp_turn               ;
        predkosc_ruchu1   =   tmp_leg                           ;
        predkosc_ruchu2   =   tmp_body                          ;
    } 
///////////DIRECTION//////////
  
   while (true)
    {

//////////TEMPERATURE/////////

         odczyt     =  analogRead(A1)                                      ;
         resistance =  seriesresistor * ((1023.0/odczyt )-1 )              ;
         logR2      =  log(resistance)                                     ;
         T          =  (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2))      ;
         Tc         =  T -  273.15                                         ;
         Serial.println (Tc)                                               ; 
     
///////////////////////////////

//////OSWIETLENIE_KAMERY///////

         analogWrite (ledpin,power)                             ;

         LightResistant = analogRead(A2)                        ;

         if (LightResistant < 100)
          {
             power++                                            ;
          }
         if (LightResistant > 100)
          {
             power=0                                            ;
          }
         if (power>255)
          {
             power=255                                          ;
          }
         if (power<0)
          {
             power=0                                            ;
          }
  

////////////////////////////    
         if (Serial.available() > 0)
          {
               wart = Serial.read()                             ;
            switch (wart)
             {
                case 119 :
                   Serial.println("Do przodu")                  ;
                   krok_naprzod(5)                              ;
                   false                                        ;
                   break                                        ;
      
                case 115 :
                   Serial.println("Do tylu")                    ;
                   krok_w_tyl (5)                               ;
                   false                                        ;
                   break                                        ;
     
                case 97 :
                   Serial.println("W prawo")                    ;
                   skrec_w_lewo (5)                             ;
                   false                                        ; 
                   break                                        ;
            
                case 100 :
                   Serial.println("W lewo")                     ;
                   skrec_w_prawo (5)                            ;
                   false                                        ;
                   break                                        ;
                    
                case 113 :
                   Serial.println("Stoj")                       ;
                   stand ()                                     ;
                   false                                        ;
                   break                                        ;  
                
                case 101 :
                   Serial.println("Siedz")                      ;
                   sit ()                                       ;
                   false                                        ;
                   break                                        ; 
             }
          }
        if (digitalRead(przod)== LOW)
         {
           krok_naprzod (5)                                     ;
           false                                                ;
         }                 
        else if (digitalRead(tyl)== LOW)
         {
           krok_w_tyl (5)                                       ;
           false                                                ;
         }
        else if (digitalRead(prawo)== LOW)
         {
           krok_w_tyl (5)                                       ;
           false                                                ;
         }
        else if (digitalRead(lewo)== LOW)
         {
           krok_w_tyl (5)                                       ;
           false                                                ;
         }
        else if (digitalRead(stoj)== LOW)
         {
           stand ()                                             ;
           false                                                ;
         }
        else if (digitalRead(siedz)== LOW)
         {
           sit ()                                               ;
           false                                                ;
         }
    } 
//////////////////////////////
}
void ustawienie(int leg, float x, float y, float z)
  {
     float length_x = 0, length_y = 0, length_z = 0 ;

     if (x != USTAW)
        length_x = x - polozenie_teraz[leg][0]  ;
     if (y != USTAW)
        length_y = y - polozenie_teraz[leg][1]  ;
     if (z != USTAW)
        length_z = z - polozenie_teraz[leg][2]  ;

     float length = sqrt(pow(length_x, 2) + pow(length_y, 2) + pow(length_z, 2)) ;

     speede[leg][0] = length_x / length * predkosc_ruchu * mnoznik_predkosci     ;
     speede[leg][1] = length_y / length * predkosc_ruchu * mnoznik_predkosci     ;
     speede[leg][2] = length_z / length * predkosc_ruchu * mnoznik_predkosci     ;

     if (x != USTAW)
        polozenie_wlasciwe[leg][0] = x                     ;
     if (y != USTAW)
        polozenie_wlasciwe[leg][1] = y                     ;
     if (z != USTAW)
        polozenie_wlasciwe[leg][2] = z                     ;
  }


void stand(void)
   {
      predkosc_ruchu = predkosc_siedz_wstan                ;
      for (int leg = 0; leg < 4; leg++)
           {
              ustawienie(leg, USTAW, USTAW, z_default)     ;
           }
      czekaj()                                             ;
   }

void sit(void)
  {
     predkosc_ruchu = predkosc_siedz_wstan                 ;
     for (int leg = 0; leg < 4; leg++)
           {
               ustawienie(leg, USTAW, USTAW, z_boot)       ;
           }
     czekaj()                                              ;
  }
  
bool is_stand(void)
  {
     if (polozenie_teraz[0][2] == z_default)
         return true                                       ;
     else
         return false                                      ;
  }

void czekaj(void)
  {
     for (int i = 0; i < 4; i++)
          czekaj_1(i)                                      ;
  }
  
void czekaj_1(int leg)
  {
     while (1)
           if (polozenie_teraz[leg][0] == polozenie_wlasciwe[leg][0])
               if (polozenie_teraz[leg][1] == polozenie_wlasciwe[leg][1])
                   if (polozenie_teraz[leg][2] == polozenie_wlasciwe[leg][2])
                       break;
  }

void zalacz_serwa(void)
  {
     for (int i = 0; i < 4; i++)
          {
            for (int j = 0; j < 3; j++)
                 {
                   servo[i][j].attach(servo_pin[i][j])     ;
                   delay(100)                              ;
                 }
          }
  }

void odlacz_serwa(void)
  {
     for (int i = 0; i < 4; i++)
          {
            for (int j = 0; j < 3; j++)
                  {
                     servo[i][j].detach()                  ;
                     delay(100)                            ;
                  }            
          }
   }

void Serwa(void)
  {
     sei();
     static float alpha, beta, gamma;

     for (int i = 0; i < 4; i++)
        {
         for (int j = 0; j < 3; j++)
            {
             if (abs(polozenie_teraz[i][j] - polozenie_wlasciwe[i][j]) >= 
		abs(speede[i][j]))
                polozenie_teraz[i][j] += speede[i][j]                           ;
             else
                polozenie_teraz[i][j] = polozenie_wlasciwe[i][j]                ;
            }

         kartezjanski_na_biegunowy(alpha, beta, gamma,
	 polozenie_teraz[i][0], polozenie_teraz[i][1], polozenie_teraz[i][2])   ;
          
         biegun_dla_serw(i, alpha, beta, gamma)                                 ;
        }
 
     licznik++;
  }

void kartezjanski_na_biegunowy(volatile float &alpha, volatile float &beta, volatile float &gamma, volatile float x, volatile float y, volatile float z)
  {
     float v, w                                                 ;

     w     = (x >= 0 ? 1 : -1) * (sqrt(pow(x, 2) + pow(y, 2)))  ;
     v     = w - length_c                                       ;

     alpha = atan2(z, v) + acos((pow(length_a, 2) - pow(length_b, 2) + pow(v, 2) 
     + pow(z, 2)) / 2 / length_a / sqrt(pow(v, 2) + pow(z, 2))) ;

     beta  = acos((pow(length_a, 2) + pow(length_b, 2) - pow(v, 2) - pow(z, 2)) 
     / 2 / length_a / length_b)                                 ;  

     gamma = (w >= 0) ? atan2(y, x) : atan2(-y, -x)             ;
 
     alpha = alpha / pi * 180                                   ;
     beta  = beta  / pi * 180                                   ;
     gamma = gamma / pi * 180                                   ;
   }
 
void biegun_dla_serw(int leg, float alpha, float beta, float gamma)
  {
     if (leg == 0)
         {
           alpha  = 90 - alpha  ;
           beta   = beta        ;
           gamma += 90          ;
         }
     else if (leg == 1)
         {
           alpha += 90          ;
           beta   = 180 - beta  ;
           gamma  = 90 - gamma  ;
         }
     else if (leg == 2)
         {
           alpha += 90          ;
           beta   = 180 - beta  ;
           gamma  = 90 - gamma  ;
         }
     else if (leg == 3)
         {
           alpha  = 90 - alpha  ;
           beta   = beta        ;
           gamma += 90          ;
         }

    servo[leg][0].write(alpha)  ;
    servo[leg][1].write(beta)   ;
    servo[leg][2].write(gamma)  ;
  }

void requestEvent() 
  {
    Wire.write(data)            ;
    data++                      ;
  }
  
  
/////////////USTAWIENIA_DLA_RUCHU/////////////

void krok_naprzod(unsigned int step)
  {
     predkosc_ruchu = predkosc_ruchu1                                             ;
     while (step-- > 0)
          {
             if (polozenie_teraz[2][1] == y_start)
                {
                   ustawienie(2, x_default + x_offset, y_start, z_up)             ;
                    czekaj()                                                      ;
                   ustawienie(2, x_default + x_offset, y_start + 2 * y_step, 			   z_up)                                                          ;
                    czekaj()                                                      ;
                   ustawienie(2, x_default + x_offset, y_start + 2 * y_step, 			   z_default)                                                     ;
                    czekaj()                                                      ;

                   predkosc_ruchu = predkosc_ruchu2                               ;

                   ustawienie(0, x_default + x_offset, y_start, z_default)        ;
                   ustawienie(1, x_default + x_offset, y_start + 2 * y_step, 			   z_default)                                                     ;
                   ustawienie(2, x_default - x_offset, y_start + y_step, 			   z_default)                                                     ;
                   ustawienie(3, x_default - x_offset, y_start + y_step, 			   z_default)                                                     ;
                    czekaj()                                                      ;

                   predkosc_ruchu = predkosc_ruchu1                               ;

                  ustawienie(1, x_default + x_offset, y_start + 2 * y_step, z_up) ;
                   czekaj()                                                       ;
                  ustawienie(1, x_default + x_offset, y_start, z_up)              ;
                   czekaj()                                                       ;
                  ustawienie(1, x_default + x_offset, y_start, z_default)         ;
                   czekaj()                                                       ;
                }
             else
                {
                  ustawienie(0, x_default + x_offset, y_start, z_up)              ;
                   czekaj()                                                       ;
                  ustawienie(0, x_default + x_offset, y_start + 2 * y_step, z_up) ;
                   czekaj()                                                       ;
                  ustawienie(0, x_default + x_offset, y_start + 2 * y_step, 
		  z_default)                                                      ;
                   czekaj()                                                       ;

                  predkosc_ruchu = predkosc_ruchu2                                ;

                  ustawienie(0, x_default - x_offset, y_start + y_step,      		  			  z_default)                                                      ;
                  ustawienie(1, x_default - x_offset, y_start + y_step, 
		  z_default)                                                      ;
                  ustawienie(2, x_default + x_offset, y_start, z_default)         ;
                  ustawienie(3, x_default + x_offset, y_start + 2 * y_step, 
		  z_default)                                                      ;
                   czekaj()                                                       ;
 
                  predkosc_ruchu = predkosc_ruchu1                                ;

                  ustawienie(3, x_default + x_offset, y_start + 2 * y_step, z_up) ;
                   czekaj()                                                       ;
                  ustawienie(3, x_default + x_offset, y_start, z_up)              ;
                   czekaj()                                                       ;
                  ustawienie(3, x_default + x_offset, y_start, z_default)         ;
                   czekaj()                                                       ;
                }
         }
  }

void krok_w_tyl(unsigned int step)
  {
     predkosc_ruchu = predkosc_ruchu1                                             ;
     while (step-- > 0)
          {
             if (polozenie_teraz[3][1] == y_start)
                {
                  ustawienie(3, x_default + x_offset, y_start, z_up)              ;
                   czekaj()                                                       ;
                  ustawienie(3, x_default + x_offset, y_start + 2 * y_step, z_up) ;
                   czekaj()                                                       ;
                  ustawienie(3, x_default + x_offset, y_start + 2 * y_step, 
		  z_default)                                                      ;
                   czekaj()                                                       ; 

                  predkosc_ruchu = predkosc_ruchu2                                ;

                  ustawienie(0, x_default + x_offset, y_start + 2 * y_step, 
		  z_default)                                                      ;
                  ustawienie(1, x_default + x_offset, y_start, z_default)         ;
                  ustawienie(2, x_default - x_offset, y_start + y_step, 
		  z_default)                                                      ;
                  ustawienie(3, x_default - x_offset, y_start + y_step, 
		  z_default)                                                      ;
                   czekaj()                                                        ;

                  predkosc_ruchu = predkosc_ruchu1                                ;

                  ustawienie(0, x_default + x_offset, y_start + 2 * y_step, z_up) ;
                   czekaj()                                                       ;
                  ustawienie(0, x_default + x_offset, y_start, z_up)              ;
                   czekaj()                                                       ;
                  ustawienie(0, x_default + x_offset, y_start, z_default)         ;
                   czekaj()                                                       ;
                 } 
              else
                 {
                  ustawienie(1, x_default + x_offset, y_start, z_up)              ;
                   czekaj()                                                       ;
                  ustawienie(1, x_default + x_offset, y_start + 2 * y_step, z_up) ;
                   czekaj()                                                       ;
                  ustawienie(1, x_default + x_offset, y_start + 2 * y_step, 
		  z_default)                                                      ;
                   czekaj()                                                       ;

                  predkosc_ruchu = predkosc_ruchu2                                ;

                  ustawienie(0, x_default - x_offset, y_start + y_step, 
		  z_default)                                                      ;
                  ustawienie(1, x_default - x_offset, y_start + y_step, 
		  z_default)                                                      ;
                  ustawienie(2, x_default + x_offset, y_start + 2 * y_step, 
		  z_default)                                                      ;
                  ustawienie(3, x_default + x_offset, y_start, z_default)         ;
                   czekaj()                                                       ;

                  predkosc_ruchu = predkosc_ruchu1                                ;

                  ustawienie(2, x_default + x_offset, y_start + 2 * y_step, z_up) ;
                   czekaj()                                                       ;
                  ustawienie(2, x_default + x_offset, y_start, z_up)              ;
                   czekaj()                                                       ;
                  ustawienie(2, x_default + x_offset, y_start, z_default)         ;
                   czekaj()                                                       ;
                 }
          }
  }

void skrec_w_prawo(unsigned int step)
  {
     predkosc_ruchu = predkosc_obrotu_miejscowego                         ;
     while (step-- > 0)
          {
             if (polozenie_teraz[2][1] == y_start)
                {
                  ustawienie(2, x_default + x_offset, y_start, z_up)      ;
                   czekaj()                                               ;

                  ustawienie(0, turn_x0 - x_offset, turn_y0, z_default)   ;
                  ustawienie(1, turn_x1 - x_offset, turn_y1, z_default)   ;
                  ustawienie(2, turn_x0 + x_offset, turn_y0, z_up)        ;
                  ustawienie(3, turn_x1 + x_offset, turn_y1, z_default)   ;
                   czekaj()                                               ;

                  ustawienie(2, turn_x0 + x_offset, turn_y0, z_default)   ;
                   czekaj()                                               ;

                  ustawienie(0, turn_x0 + x_offset, turn_y0, z_default)   ;
                  ustawienie(1, turn_x1 + x_offset, turn_y1, z_default)   ;
                  ustawienie(2, turn_x0 - x_offset, turn_y0, z_default)   ;
                  ustawienie(3, turn_x1 - x_offset, turn_y1, z_default)   ;
                   czekaj()                                               ;

                  ustawienie(0, turn_x0 + x_offset, turn_y0, z_up)        ;
                   czekaj()                                               ; 

                  ustawienie(0, x_default + x_offset, y_start, z_up)      ;
                  ustawienie(1, x_default + x_offset, y_start, z_default) ;
                  ustawienie(2, x_default - x_offset, y_start + y_step, 
		  z_default)                                              ;
                  ustawienie(3, x_default - x_offset, y_start + y_step, 
		  z_default)                                              ;
                   czekaj()                                               ;

                  ustawienie(0, x_default + x_offset, y_start, z_default) ;
                   czekaj()                                               ;
                 }
             else
                 {
                  ustawienie(1, x_default + x_offset, y_start, z_up)      ;
                   czekaj()                                               ;

                  ustawienie(0, turn_x1 + x_offset, turn_y1, z_default)   ;
                  ustawienie(1, turn_x0 + x_offset, turn_y0, z_up)        ;
                  ustawienie(2, turn_x1 - x_offset, turn_y1, z_default)   ;
                  ustawienie(3, turn_x0 - x_offset, turn_y0, z_default)   ;
                   czekaj()                                               ;

                  ustawienie(1, turn_x0 + x_offset, turn_y0, z_default)   ;
                   czekaj()                                               ; 

                  ustawienie(0, turn_x1 - x_offset, turn_y1, z_default)   ;
                  ustawienie(1, turn_x0 - x_offset, turn_y0, z_default)   ;
                  ustawienie(2, turn_x1 + x_offset, turn_y1, z_default)   ;
                  ustawienie(3, turn_x0 + x_offset, turn_y0, z_default)   ;
                   czekaj()                                               ;

                  ustawienie(3, turn_x0 + x_offset, turn_y0, z_up)        ;
                   czekaj()                                               ;

                  ustawienie(0, x_default - x_offset, y_start + y_step, 
		  z_default)                                              ;
                  ustawienie(1, x_default - x_offset, y_start + y_step, 
		  z_default)                                              ;
                  ustawienie(2, x_default + x_offset, y_start, z_default) ;
                  ustawienie(3, x_default + x_offset, y_start, z_up)      ;
                   czekaj()                                               ;

                  ustawienie(3, x_default + x_offset, y_start, z_default) ;
                   czekaj()                                               ;
                 }
           }
  }
void skrec_w_lewo(unsigned int step)
  {
     predkosc_ruchu = predkosc_obrotu_miejscowego                         ;
     while (step-- > 0)
          {
             if (polozenie_teraz[3][1] == y_start)
                {
                  ustawienie(3, x_default + x_offset, y_start, z_up)      ;
                   czekaj()                                               ; 

                  ustawienie(0, turn_x1 - x_offset, turn_y1, z_default)   ;
                  ustawienie(1, turn_x0 - x_offset, turn_y0, z_default)   ;
                  ustawienie(2, turn_x1 + x_offset, turn_y1, z_default)   ;
                  ustawienie(3, turn_x0 + x_offset, turn_y0, z_up)        ;
                   czekaj()                                               ;

                  ustawienie(3, turn_x0 + x_offset, turn_y0, z_default)   ;
                   czekaj()                                               ;

                  ustawienie(0, turn_x1 + x_offset, turn_y1, z_default)   ;
                  ustawienie(1, turn_x0 + x_offset, turn_y0, z_default)   ;
                  ustawienie(2, turn_x1 - x_offset, turn_y1, z_default)   ;
                  ustawienie(3, turn_x0 - x_offset, turn_y0, z_default)   ;
                   czekaj()                                               ;

                  ustawienie(1, turn_x0 + x_offset, turn_y0, z_up)        ;
                   czekaj()                                               ;

                  ustawienie(0, x_default + x_offset, y_start, z_default) ;
                  ustawienie(1, x_default + x_offset, y_start, z_up)      ;
                  ustawienie(2, x_default - x_offset, y_start + y_step, 
		  z_default)                                              ;
                  ustawienie(3, x_default - x_offset, y_start + y_step, 
		  z_default)                                              ;
                   czekaj()                                               ;

                  ustawienie(1, x_default + x_offset, y_start, z_default) ;
                   czekaj()                                               ;
                }
             else
                {
                  ustawienie(0, x_default + x_offset, y_start, z_up)      ;
                   czekaj()                                               ;
 
                  ustawienie(0, turn_x0 + x_offset, turn_y0, z_up)        ;
                  ustawienie(1, turn_x1 + x_offset, turn_y1, z_default)   ;
                  ustawienie(2, turn_x0 - x_offset, turn_y0, z_default)   ;
                  ustawienie(3, turn_x1 - x_offset, turn_y1, z_default)   ;
                   czekaj()                                               ;

                  ustawienie(0, turn_x0 + x_offset, turn_y0, z_default)   ;
                   czekaj()                                               ;

                  ustawienie(0, turn_x0 - x_offset, turn_y0, z_default)   ;
                  ustawienie(1, turn_x1 - x_offset, turn_y1, z_default)   ;
                  ustawienie(2, turn_x0 + x_offset, turn_y0, z_default)   ;
                  ustawienie(3, turn_x1 + x_offset, turn_y1, z_default)   ;
                   czekaj()                                               ;

                  ustawienie(2, turn_x0 + x_offset, turn_y0, z_up)        ;
                   czekaj()                                               ;

                  ustawienie(0, x_default - x_offset, y_start + y_step, 
		  z_default)                                              ;
                  ustawienie(1, x_default - x_offset, y_start + y_step, 
		  z_default)                                              ;
                  ustawienie(2, x_default + x_offset, y_start, z_up)      ;
                  ustawienie(3, x_default + x_offset, y_start, z_default) ;
                   czekaj()                                               ;

                  ustawienie(2, x_default + x_offset, y_start, z_default) ;
                   czekaj()                                               ;
                }
          }
  }
