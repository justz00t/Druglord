#include <dos.h>

void sfx(id)
int id;
{
    register int i, j;

    switch (id) {
    case 2:
        sound(2000); delay(10);
        sound(5000); delay(20);
        sound(500);  delay(30);
        sound(1000); delay(40);
        break;
    case 3:
        for (i = 500; i > 200; i -= 100) {
            sound(i);       delay(100);
            sound(i - 200); delay(100);
            nosound();      delay(50);
        }
        break;
    case 4:
        for (i = 500; i < 1000; i += 100) {
            sound(i);        delay(10);
            sound(i + 2000); delay(20);
            nosound();       delay(10);
        }
        break;
    case 5:
        for (j = 0; j < 6; j++) {
            for (i = 7; i < 15; i++) {
                sound(i * 100); delay(25);
            }
            for (i = 15; i > 7; i--) {
                sound(i * 100); delay(25);
            }
        }
        break;
    case 6:
        for (i = 0; i < 5; i++)
            for (j = 0; j < 10; j++) {
                sound(i * 1000 + j * 1000); delay(40);
            }
        break;
    case 7:
        for (i = 100; i > 0; i--) {
            sound(5000 - i * 50); delay(5);
            sound(i * 50 + 5000); delay(5);
        }
        break;
    case 8:
        sound(2000); delay(30);
        sound(500);  delay(25);
        sound(70);   delay(40);
        break;
    case 11:
        sound(1000); delay(50);
        break;
    case 12:
        sound(500); delay(30);
        sound(200); delay(20);
        break;
    case 13:
        sound(100); delay(30);
        sound(50);  delay(30);
        break;
    case 14:
        for (i = 1; i < 2; i++) {
            sound(150); delay(30);
            sound(100); delay(50);
        }
        break;
    case 15:
        for (i = 1; i < 3; i++) {
            sound(100); delay(20);
            sound(50);  delay(60);
        }
        break;
    case 20:
        sound(800); delay(100);
        break;
    }
    nosound();
}
