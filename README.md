# nodos-IoT-desarrollados
Código de programación de los dispositivos inteligentes desarrollados para un sistema de hogar inteligente.

Los siguientes nodos se programaron en Arduino IDE:  
•	Cámara  
•	Control de riego  
•	Control parlante  
•	Control remoto IR genérico  
•	Nodo de notificaciones  
•	Nodo detector de gases  

El nodo control de acceso tiene un microcontrolador PIC que se encuentra programado en C y envía los comandos AT para interactuar con el módulo ESP01, el cual brinda la conectividad inalámbrica.

El nodo control de iluminación está implementado en la placa de desarrollo SEA Board. Tiene la programación en el entorno Arduino para el chip ESP32 y la programación en VHDL para la FPGA.


