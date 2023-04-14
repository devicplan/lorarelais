# battery-powered LoRaWAN relay with many expansion possibilities

The circuit of the LoRaWan relay uses a bistable relay. This type of relay requires only a short current pulse to be either on or off. Common monostable relays need a not insignificant current to stay switched on. While it is considerably more costly to use such relays in circuits, the energy benefit always outweighs the cost with battery powered devices. Already with Regenmesser I had to spend a lot of time to find the needed Lora Radio Node V1.0. For this reason I developed a suitable PCB with battery holder (for LiPo or even better LiFePo4 batteries) myself, which contains all components of the relay control and still has a lot of space for own ideas.

The software uses the LoRaWan device class Class A. After each transmission there is a small time window to receive any data packets. These data packets to be received are a) the desired relay switching state or b) the time span when to send/receive again. As time span between individual sending processes 1 to 180 minutes are possible. If you have selected e.g. one hour as transmission interval, not only the data come in the hour interval from the LoRaWan node, but this also does not react to sent switching commands sooner. If one wants to execute switching operations in the coming time, then one could reduce the transmission interval time for a while, in order to increase the reaction time. If you can live with the fact that switching operations will be executed in the next 3 hours, you can also keep the most energy-saving setting with a send interval every 180 minutes.

When sending a single byte, the relay is switched off at 0 and switched on at any other value 0x01-0xFF.

When sending a double byte, the first byte contains the send interval as a hex number and the second byte contains the desired relay switching function. Sending 0x01 0x10 would set up the time of 16 minutes (0x10) as future send intervals and switch on the relay with 0x01. ATTENTION ! There are no shorter send intervals of 1 minute and no longer than 180 minutes. So sending 0x00 0xFF would only set up the send interval to 180 minutes and turn off the relay.

At each send interval 6 bytes are transmitted. The 6 bytes are currently used as follows.

1. Byte - 0x00 (relay is off) or 0x01 (relay is on)
2. Byte - 0x01...0xB4 (transmission interval in minutes 1-180 minutes)
3. Byte - MSB voltage of the accumulator (0x03 = 3.xxx Volt)
4. Byte - first decimal place voltage (0x07 = x.7xx Volt)
5. Byte - second decimal place voltage (0x04 = x.x4x volts)
6. Byte - LSB or third decimal place voltage (0x09 = x.xx9 volts)

I chose the simple voltage output over the last 4 bytes, because I wanted to read the voltage of the accumulator fast and without calculating first.

read more at https://icplan.de/seite43/

# batteriebetriebenes LoRaWAN-Relais mit vielen Erweiterungsmöglichkeiten

Die Schaltung vom LoRaWan Relais nutzt ein bistabiles Relais. Dieser Typ von Relais benötig nur einen kurzen Stromimpuls um entweder ein- oder ausgeschaltet zu sein. Übliche monostabile Relais benötigen einen nicht unerheblichen Strom um eingeschalten zu bleiben. Zwar ist es erheblich aufwändiger solche Relais in Schaltungen zu verwenden, doch der energetische Nutzen überwiegt bei batteriebetrieben Geräten immer. Schon bei Regenmesser musste ich viel Zeit aufbringen um das benötigte Lora Radio Node V1.0 aufzutreiben. Aus diesem Grund habe ich selbst eine passende Leiterplatte mit Batteriehalter (für LiPo oder besser noch LiFePo4 Akkus) entwickelt, die alle Bauteile der Relaissteuerung enthält und noch viel Platz für eigene Ideen hat.

Die Software nutzt die LoRaWan Geräteklasse Class A. Nach jedem Senden gibt es kleines Zeitfenster um eventuell vorhandene Datenpakete zu empfangen. Diese zu empfangenden Datenpakete sind a) der gewünschte Relaisschaltzustand oder b) die Zeitspanne wann wieder gesendet/empfangen werden soll. Als Zeitspanne zwischen einzelnen Sendevorgängen sind 1 bis 180 Minuten möglich. Hat man als Sendeintervall z.B. eine Stunde gewählt kommen nicht nur die Daten im Stundenintervall vom LoRaWan Knoten, sonder dieser reagiert auch auf gesendete Schaltbefehle nicht eher. Will man in der kommenden Zeit Schaltvorgänge ausführen, dann könnte man eine Zeitlang die Sendeintervallzeit verringern, um damit Reaktionszeit zu erhöhen. Wer damit leben kann, dass in den kommenden 3 Stunden der Schaltvorgang ausgeführt wird, kann auch die energiesparendste Einstellung mit einem Sendeintervall aller 180 Minuten beibehalten.

Beim Senden eines einzelnen Bytes wird bei 0 das Relais ausgeschaltet und bei jedem anderen Wert 0x01-0xFF eingeschaltet.

Beim Senden eines Doppelbyte enthält das erste Byte den Sendeintervall als Hexzahl und im zweiten Byte die gewünschte Relaisschaltfunktion. Das Senden von 0x01 0x10 würde als künftige Sendeintervalle die Zeit von 16 Minuten (0x10) einrichten und das Relais mit der 0x01 einschalten. ACHTUNG ! Es gibt keine kürzeren Sendintervalle von 1 Minute und keine längeren als 180 Minuten. Das Senden von 0x00 0xFF würde also nur den Sendeintervall auf 180 Minuten einrichten und das Relais aussschalten.

Bei jedem Sendeintervall werden 6 Byte übertragen. Die 6 Byte werden momentan wie folgt genutzt.

1. Byte – 0x00 (Relais ist aus) oder 0x01 (Relais ist an)
2. Byte – 0x01…0xB4 (Sendeintervall in Minuten 1-180 Minuten)
3. Byte – MSB Spannung des Akkus (0x03 = 3.xxx Volt)
4. Byte – erste Nachkommastelle Spannung (0x07 = x.7xx Volt)
5. Byte – zweite Nachkommastelle Spannung (0x04 = x.x4x Volt)
6. Byte – LSB oder dritte Nachkommastelle Spannung (0x09 = x.xx9 Volt)

Die simple Spannungsausgabe über die letzten 4 Byte habe ich gewählt, weil ich schnell und ohne erst rumzurechnen die Spannung des Akkus ablesen wollte.

Weiterlesen unter https://icplan.de/seite43/
