/***************************************************
код предназначен для платы MEGA PRO MINI
дисплея 2,8"
датчика CO2 MH-Z14A
датчик подключается к Serial 3
 ****************************************************/


#include "SPI.h"
#include "Adafruit_GFX.h"       //универсальная графическая библиотека
#include "Adafruit_ILI9341.h"   //драйвер конкретной платы

// Пины для подключения дисплея
#define TFT_DC 48
#define TFT_CS 49
#define TFT_MOSI 11
#define TFT_MISO 12
#define TFT_CLK 13
#define TFT_RST 8

/* Предустановленные цвета
ILI9341_RED
ILI9341_BLUE
ILI9341_GREEN
ILI9341_YELLOW
ILI9341_WHITE
ILI9341_CYAN
ILI9341_BLACK
ILI9341_MAGENTA
 */

// Использование hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC RESET подключить к RESET на плате
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
// Можно установить любые пины
// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

//переменные для датчика CO2 
byte cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};// запрос уровня газа
byte cmdAutoCalOn[9] = {0xFF,0x01,0x79,0xA0,0x00,0x00,0x00,0x00,0xE6};// автокалибровка включена
byte cmdAutoCalOff[9] = {0xFF,0x01,0x79,0x00,0x00,0x00,0x00,0x00,0x86};// автокалибровка вЫключена
byte cmdZeroPoint[9] = {0xFF,0x01,0x87,0x00,0x00,0x00,0x00,0x00,0x78};// устновка значения 400ppm на чистом воздухе
unsigned char response[9];//переменная для приема ответа датчика

//Глобальные переменные
int co21min [181];  //массив значения для верхнего графика с шагом 1 мин
int co21minMax = 0;     //максималное значение для верхнего графика
int co23min [281];  //массив значения для нижнего графика с шагом 3 мин
int co23minMax = 0;     //максималное значение для нижнего графика
byte flag3min = 1;	//флаг для отчета каждой 3-ей минуты
unsigned long my_timer; //переменная для хранения времени

////////////////////////////////////////////
///////Функция перезагрузки
void (* resetFunc) (void) = 0;

void setup() {
  //digitalWrite(52, HIGH); //включаем подтягивающий резистр на выход CLK(SCK)
  
  Serial.begin(9600);
  Serial.println("Test!"); 
  Serial3.begin (9600);//включаем Serial 3 к которому подключен датчик к D14->RX MH-Z14A, к D15->TX MH-Z14A
  tft.begin(); // инициализируем экран
  tft.setRotation(3); // задать поворот экрана от 0 до 3
  //delay(10000);
  my_timer = millis();   // "сбросить" таймер
  otschet (30); //обратный отсчет 
  //Serial3.write(cmdAutoCalOn, 9); //автокалибровка MH-Z14A включена
  Serial3.write(cmdAutoCalOff, 9); //автокалибровка MH-Z14A вЫключена
  /*
  if ( getCO2()==0){
    Serial.println("Test MH-Z14A - ERROR"); 
    tft.fillScreen(ILI9341_BLACK); // очистка экрана, заливка черным
    tft.setCursor(50, 75); //установка курсора
    tft.setTextColor(tft.color565(255, 0, 0));//настройка текста Цвет КРАСНЫЙ
    tft.setTextSize(1);
    tft.print("Test MH-Z14A - ERROR");  //вывод текста на экран
    delay(3000);
    resetFunc(); //если датчик выдает ошибку, перезагружаемся
  }
  delay(1000);
  Serial.println("Start"); 
  tft.fillScreen(ILI9341_BLACK); // очистка экрана, заливка черным
  tft.setCursor(50, 75); //установка курсора
  tft.setTextColor(tft.color565(40, 255, 0));//настройка текста Цвет ЗЕЛЕНЫЙ
  tft.setTextSize(1);
  tft.print("Test MH-Z14A - OK");  //вывод текста на экран
  delay(3000);
*/
  //заполняем экран фоном
  fon(co21min[0], co21minMax, co23minMax); // текущее содержание СО2, максимальное содержание СО2 на верхнем экране, максимальное содержание СО2 на нижнем экране
  delay(350);
}


void loop(void) {

  //digitalWrite(52, HIGH); //включаем подтягивающий резистр на выход CLK(SCK)без него иногда отключается CLK(SCK)
  
  //testText();
  //delay(1000);
  //testFastLines(ILI9341_RED, ILI9341_BLUE);
  //delay(1000);
  //int co2 = getCO2(); // получаем и выводим показания в Serial;
  if (millis() - my_timer >= 30000){  //если прошло 30 секунд
    my_timer = millis();   // "сбросить" таймер
    addMassivCo2();
    fon(co21min[0], co21minMax, co23minMax); // текущее содержание СО2, максимальное содержание СО2 на верхнем экране, максимальное содержание СО2 на нижнем экране
    grafiki();
  }
  //addMassivCo2();
  //fon(co21min[0], co21minMax, co23minMax); // текущее содержание СО2, максимальное содержание СО2 на верхнем экране, максимальное содержание СО2 на нижнем экране
  //grafiki();
  //delay(15000);
}
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
///////Фоновое изображение////////////////////////////////
void fon (int co2, int co2max1, int co2max2){
  //int co2 = 320; // текущее содержание СО2
  //int co2max1 = 4500; // максимальное содержание СО2 на верхнем экране
  //int co2max2 = 4950; // максимальное содержание СО2 на нижнем экране
  int m1 = mashtab (co2max1); //расчет масштаба
  int m2 = mashtab (co2max2); //расчет масштаба
  
  tft.fillScreen(ILI9341_BLACK); // очистка экрана, заливка черным
  //////////////////////////
  ///////нижний блок////////
  for(int i=0; i<15; i++){  //рисование вертикальных линий внизу
    int dx=20;  //приращение
    int sx=39;  //смещение
    dx = dx * i +sx;
    tft.drawFastVLine(dx, 130, 104, tft.color565(70, 70, 70)); //X, Y, длина, цветRGB
    tft.setCursor(dx-5, 120); //установка курсора
    tft.setTextColor(ILI9341_BLUE);  tft.setTextSize(1);  //настройка текста Цвет и размер
    int dval = 14 - i;
    tft.println(dval);  //вывод текста на экран
  }
  for(int i=0; i<6; i++){   //рисование горизонтальных линий внизу
    int dy=-20;  //приращение
    int sy=233;  //смещение
    dy = dy * i +sy;
    tft.drawFastHLine(35, dy, 320, tft.color565(30, 30, 70)); //X, Y, длина, цвет
    tft.setCursor(8, dy-3); //установка курсора
    tft.setTextColor(ILI9341_BLUE);  tft.setTextSize(1);  //настройка текста Цвет и размер
    int dval = 400 + i*100*m2;
    tft.println(dval);  //вывод текста на экран
  }
  //линия внизу обозначающая 90 минут
  tft.drawFastVLine(289, 130, 104, tft.color565(90, 90, 170)); //X, Y, длина, цветRGB
  ///////////////////////////
  ///////верхний блок////////
  for(int i=0; i<7; i++){  //рисование вертикальных линий внизу
    int dx=30;  //приращение
    int sx=139;  //смещение
    dx = dx * i +sx;
    tft.drawFastVLine(dx, 10, 102, tft.color565(70, 70, 70)); //X, Y, длина, цветRGB
    tft.setCursor(dx-5, 0); //установка курсора
    tft.setTextColor(ILI9341_BLUE);  tft.setTextSize(1);  //настройка текста Цвет и размер
    int dval = 90 - i*15;
    tft.println(dval);  //вывод текста на экран
  }
  for(int i=0; i<6; i++){   //рисование горизонтальных линий внизу
    int dy=-20;  //приращение
    int sy=112;  //смещение
    dy = dy * i +sy;  //установка курсора
    tft.drawFastHLine(135, dy, 320, tft.color565(30, 30, 70)); //X, Y, длина, цвет
    tft.setCursor(110, dy-3);
    tft.setTextColor(ILI9341_BLUE);  tft.setTextSize(1);  //настройка текста Цвет и размер
    int dval = 400 + i*100*m1;
    tft.println(dval);  //вывод текста на экран
  }
  //////////////////////
  ///////Надписи////////
  tft.setCursor(5, 5); //установка курсора
  if (co2<600){tft.setTextColor(tft.color565(40, 255, 0));}  //настройка текста Цвет ЗЕЛЕНЫЙ до 600
  else if (co2<1000){tft.setTextColor(tft.color565(240, 255, 0));}  //настройка текста Цвет ЖЕЛТЫЙ до 1000
  else if (co2<2000){tft.setTextColor(tft.color565(255, 150, 50));}  //настройка текста Цвет ОРАНЖЕВЫЙ до 2000
  else if (co2<3000){tft.setTextColor(tft.color565(255, 50, 50));}  //настройка текста Цвет КРАСНЫЙ до 3000
  else {tft.setTextColor(tft.color565(180, 0, 110));}  //настройка текста Цвет БОРДОВЫЙ от 3000
  
  tft.setTextSize(2);  //настройка текста  размер
  tft.println("CO2");  //вывод текста на экран
  tft.setCursor(5, 25); //установка курсора
  tft.setTextSize(3);
  tft.println(co2);  //вывод текста на экран

  tft.setCursor(5, 70); //установка курсора
  if (co2max1<600){tft.setTextColor(tft.color565(40, 255, 0));}  //настройка текста Цвет ЗЕЛЕНЫЙ до 600
  else if (co2max1<1000){tft.setTextColor(tft.color565(240, 255, 0));}  //настройка текста Цвет ЖЕЛТЫЙ до 1000
  else if (co2max1<2000){tft.setTextColor(tft.color565(255, 150, 50));}  //настройка текста Цвет ОРАНЖЕВЫЙ до 2000
  else if (co2max1<3000){tft.setTextColor(tft.color565(255, 50, 50));}  //настройка текста Цвет КРАСНЫЙ до 3000
  else {tft.setTextColor(tft.color565(180, 0, 110));}  //настройка текста Цвет БОРДОВЫЙ от 3000
  //tft.setTextColor(tft.color565(255, 50, 70));  tft.setTextSize(1);  //настройка текста Цвет и размер
  tft.setTextSize(1);
  tft.print("CO2 max1=");  //вывод текста на экран
  tft.println(co2max1);  //вывод текста на экран

  tft.setCursor(5, 90); //установка курсора
  if (co2max2<600){tft.setTextColor(tft.color565(40, 255, 0));}  //настройка текста Цвет ЗЕЛЕНЫЙ до 600
  else if (co2max2<1000){tft.setTextColor(tft.color565(240, 255, 0));}  //настройка текста Цвет ЖЕЛТЫЙ до 1000
  else if (co2max2<2000){tft.setTextColor(tft.color565(255, 150, 50));}  //настройка текста Цвет ОРАНЖЕВЫЙ до 2000
  else if (co2max2<3000){tft.setTextColor(tft.color565(255, 50, 50));}  //настройка текста Цвет КРАСНЫЙ до 3000
  else {tft.setTextColor(tft.color565(180, 0, 110));}  //настройка текста Цвет БОРДОВЫЙ от 3000
  //tft.setTextColor(tft.color565(255, 0, 0));  tft.setTextSize(1);  //настройка текста Цвет и размер
  tft.print("CO2 max2=");  //вывод текста на экран
  tft.setTextSize(1);
  tft.println(co2max2);  //вывод текста на экран
}
//////////////////////////////
////////расчет масштаба///////
int mashtab (int co2){
  int m = (co2-400)/500+1;
  return m;
}
//////////////////////////////
///////обратный отсчет при старте
void otschet (int t){
	tft.fillScreen(ILI9341_BLACK); // очистка экрана, заливка черным
	tft.setCursor(50, 75); //установка курсора
	tft.setTextColor(tft.color565(40, 255, 0));//настройка текста Цвет ЗЕЛЕНЫЙ
	tft.setTextSize(1);
	tft.print("Preparation of the gas sensor MH-Z14A");  //вывод текста на экран
	for (int i=0; i<t; i++){
		tft.fillRect(140, 100, 50,30, ILI9341_BLACK); //рисуем прямоугольник, закрашиваем прошлые цифры x,y,ширина,высота, цвет
		tft.setCursor(140, 100); //установка курсора
		tft.setTextColor(tft.color565(40, 255, 0));//настройка текста Цвет ЗЕЛЕНЫЙ
		tft.setTextSize(4);
		tft.println(t-i);  //вывод текста на экран
		delay (1000);
	}
  
}



////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
unsigned long testFillScreen() {
  unsigned long start = micros();
  tft.fillScreen(ILI9341_BLACK);
  yield();
  tft.fillScreen(ILI9341_RED);
  yield();
  tft.fillScreen(ILI9341_GREEN);
  yield();
  tft.fillScreen(ILI9341_BLUE);
  yield();
  tft.fillScreen(ILI9341_BLACK);
  yield();
  return micros() - start;
}

unsigned long testText() {
  tft.fillScreen(ILI9341_BLACK);
  unsigned long start = micros();
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(ILI9341_YELLOW); tft.setTextSize(2);
  tft.println(1234.56);
  tft.setTextColor(ILI9341_RED);    tft.setTextSize(3);
  tft.println(0xDEADBEEF, HEX);
  tft.println();
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(5);
  tft.println("Groop");
  tft.setTextSize(2);
  tft.println("I implore thee,");
  tft.setTextSize(1);
  tft.println("my foonting turlingdromes.");
  tft.println("And hooptiously drangle me");
  tft.println("with crinkly bindlewurdles,");
  tft.println("Or I will rend thee");
  tft.println("in the gobberwarts");
  tft.println("with my blurglecruncheon,");
  tft.println("see if I don't!");
  return micros() - start;
}
////////////////////////////////////////////////////////////////////////////////////
////Функции датчика CO2/////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////
///////чтение показаний датчика MH-Z14A
int getCO2() 
{
  int co2 = 0;
  //SerialCO2.write(cmd, 9); //отправляем команду датчику
  Serial3.write(cmd, 9); //отправляем команду датчику
  memset(response, 0, 9);
  //SerialCO2.readBytes(response, 9); //получаем ответ
  Serial3.readBytes(response, 9); //получаем ответ
  
  uint8_t crc = 0;
  for (int i = 1; i < 8; i++) crc+=response[i]; //считаем контрольную сумму
  crc = ~crc;
  crc++;
  

  if ( !(response[0] == 0xFF && response[1] == 0x86 && response[8] == crc) ) // сравниваем ответ с  контрольной суммой
  {
     Serial.println("CRC error: " + String(crc) + " / "+ String(response[8])); //выводим ошибку
	   
	   //исправление ошибки связанной с переполнением буфера
	   while (Serial3.available()) {   //если в буфере остались файлы
		  Serial3.read();				//то читаем отсавшийся файл
      Serial.println("CRC error: чистим буфер");
	   }
     

  } 
  else 
  {
    uint8_t responseHigh = (uint8_t) response[2];
    uint8_t responseLow = (uint8_t) response[3];
    Serial.println("CO2 "+String((256*responseHigh) + responseLow)+"ppm"); //выводим показания датчика
    co2=(256*responseHigh) + responseLow;
  }
  return co2;
} 

////////////////////////////////////////////
///////заполняем массивы значениями CO2
void addMassivCo2 (void) {
	int co2=getCO2();
	//int co21min [181];  //массив значения для верхнего графика с шагом 1 мин
	//int co21minMax;     //максималное значение для верхнего графика
	//int co23min [281];  //массив значения для нижнего графика с шагом 3 мин
	//int co23minMax;     //максималное значение для нижнего графика
	//byte flag3min = 1;	//флаг для отчета каждой 3-ей минуты
	co21minMax = co2;	//максималное значение для верхнего графика
	for (int i=179; i>=0; i--){ ////сдвигаем массив на одно значение
		co21min[i+1]=co21min[i];
		//Serial.print(co21min[i]);
		//Serial.print(" - ");
		//Serial.println(co21min[i+1]);
		if (co21minMax < co21min[i+1]) co21minMax = co21min[i+1]; //если текущее значение больше, то присваиваем его максимальному
	}
	co21min[0]=co2; // вписываем текущее значение СО2 в начало массива
	if (flag3min == 1){ //если мы на первой минуте из трех, то сдвигаем массив и пишем новое значение в начало
		co23minMax = co2;	//максималное значение для нижнего графика
		for (int i=279; i>=0; i--){ ////сдвигаем массив на одно значение
			co23min[i+1]=co23min[i];
			if (co23minMax < co23min[i+1]) co23minMax = co23min[i+1]; //если текущее значение больше, то присваиваем его максимальному
		}
		///// вписываем максимальное значение за последние 3 минуты
   int max3min = 0; //максимальное значение за 3 минуты
   for (int i=0; i<6; i++){
      if (co21min[i]>max3min) max3min = co21min[i];
   }
   co23min[0] = max3min; //присваиваем максимальное значение за 3 минуты
   /*
		if (co21min[0]>co21min[1]){	//если первое значение больше второго
			co23min[0]=co21min[0];	//то присваиваем первое значение
		} else {
			co23min[0]=co21min[1];	//иначе присваиваем второе значение
		}
		if (co23min[0]<co21min[2]){	//если третье значение больше присвоиного
			co23min[0]=co21min[2];	//то переприсваиваем третье значение
		}*/
		if (co23minMax < co23min[0]) co23minMax = co23min[0]; //если текущее значение больше максимального, то присваиваем его максимальному
    
	}
	flag3min++; //увеличиваем флаг на 1
	if (flag3min>6){	//если флаг больше 6
		flag3min=1;		//то присваиваем ему 1
	}
 Serial.println(co21min[0]);
 Serial.println(co21min[1]);
 Serial.println(co21min[2]);
 Serial.println(co21min[3]);
 Serial.println(co21min[4]);
 Serial.println(co21min[5]);
 Serial.println("3min");
 Serial.println(co23min[0]);
 Serial.println(co23min[1]);
 Serial.println(co23min[2]);
 Serial.println(co23min[3]);
}

////////////////////////////////////////////
///////рисуем графики
void grafiki (void){
	//int co21min [181];  //массив значения для верхнего графика с шагом 1 мин
	//int co21minMax;     //максималное значение для верхнего графика
	//int co23min [281];  //массив значения для нижнего графика с шагом 3 мин
	//int co23minMax;     //максималное значение для нижнего графика
	//byte flag3min = 1;	//флаг для отчета каждой 3-ей минуты
	///////рисуем верхний график
	int mv = mashtab(co21minMax);	//вычисляем масштаб верхнего графика
	for (int i =0; i < 181; i++){
		int l = (co21min[i] - 400)/(5*mv);
		if (l>=0){
			//tft.drawFastVLine( 319-i,112-l, l, tft.color565(40, 255, 0));
			if (co21min[i]<600){tft.drawFastVLine( 319-i,112-l, l, (tft.color565(40, 255, 0)));}  //настройка текста Цвет ЗЕЛЕНЫЙ до 600
			else if (co21min[i]<1000){tft.drawFastVLine( 319-i,112-l, l, (tft.color565(240, 255, 0)));}  //настройка текста Цвет ЖЕЛТЫЙ до 1000
			else if (co21min[i]<2000){tft.drawFastVLine( 319-i,112-l, l, (tft.color565(255, 150, 50)));}  //настройка текста Цвет ОРАНЖЕВЫЙ до 2000
			else if (co21min[i]<3000){tft.drawFastVLine( 319-i,112-l, l, (tft.color565(255, 50, 50)));}  //настройка текста Цвет КРАСНЫЙ до 3000
			else {tft.drawFastVLine( 319-i,112-l, l, (tft.color565(180, 0, 110)));}  //настройка текста Цвет БОРДОВЫЙ от 3000
		}
		
	}
	///////рисуем нижний график
	int mn = mashtab(co23minMax);	//вычисляем масштаб нижнего графика
	for (int i =0; i < 281; i++){
		int l = (co23min[i] - 400)/(5*mn);
		if (l>=0){
			//tft.drawFastVLine( 319-i,112-l, l, tft.color565(40, 255, 0));
			if (co23min[i]<600){tft.drawFastVLine( 319-i,233-l, l, (tft.color565(40, 255, 0)));}  //настройка текста Цвет ЗЕЛЕНЫЙ до 600
			else if (co23min[i]<1000){tft.drawFastVLine( 319-i,233-l, l, (tft.color565(240, 255, 0)));}  //настройка текста Цвет ЖЕЛТЫЙ до 1000
			else if (co23min[i]<2000){tft.drawFastVLine( 319-i,233-l, l, (tft.color565(255, 150, 50)));}  //настройка текста Цвет ОРАНЖЕВЫЙ до 2000
			else if (co23min[i]<3000){tft.drawFastVLine( 319-i,233-l, l, (tft.color565(255, 50, 50)));}  //настройка текста Цвет КРАСНЫЙ до 3000
			else {tft.drawFastVLine( 319-i,233-l, l, (tft.color565(180, 0, 110)));}  //настройка текста Цвет БОРДОВЫЙ от 3000
		}
		
	}
}

////////////////////////////////////////////
///////Функция перезагрузки
//void (* resetFunc) (void) = 0;
