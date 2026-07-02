# Centralina di Allarme di Prossimità (Bare-Metal Embedded System)

Questo repository contiene il firmware e la configurazione hardware per una centralina di allarme di prossimità basata su microcontrollore ATmega328P. L'intero sistema è stato progettato seguendo un paradigma di ottimizzazione a basso livello (*Bare-Metal*), escludendo l'uso del framework standard di Arduino o di librerie esterne per la gestione delle periferiche.

## ⚙️ Come Funziona (Logica di Sistema)

Il comportamento della centralina e l'intero ciclo di vita dell'allarme sono governati da una Macchina a Stati Finiti (FSM) che risponde ai seguenti comportamenti operativi:

### 1. Rilevamento e Inserimento
* **Stato di Stand-by (Stato 0):** Il sistema è a riposo con la sirena spenta. Il sensore a ultrasuoni esegue letture cicliche ogni 100 ms.
* **Attivazione per Prossimità:** Se il sensore rileva un oggetto o un soggetto a una distanza inferiore agli 80 cm, il sistema passa allo stato di **Inserimento (Stato 1)**. Lo schermo LCD si pulisce e si avvia un timer software di **90 secondi** (900 tick da 100 ms).
* **Interazione a Pulsante Singolo:** L'inserimento del codice a 5 cifre avviene interamente tramite un solo pulsante (connesso al pin `PC0`), discriminando il tempo di pressione tra il fronte di discesa e il fronte di salita:
  * **Pressione Breve (< 1 secondo):** Incrementa in modo ciclico (da 0 a 9) il valore della cifra attualmente selezionata sul display LCD.
  * **Pressione Prolungata (> 1 secondo):** Conferma la cifra corrente e passa allo stato di **Conferma Cifra (Stato 3)**, che memorizza il valore nell'array dell'utente, incrementa il contatore delle cifre inserite e riporta il sistema in modalità inserimento per la cifra successiva.

### 2. Validazione e Gestione Allarme
* **Controllo Codice (Stato 4):** Una volta inserite e confermate tutte e 5 le cifre, il sistema confronta l'array memorizzato dall'utente con il codice segreto preimpostato (`1, 2, 3, 4, 5`).
* **Accesso Consentito (Stato 5):** Se il codice è corretto, il display mostra il messaggio di sblocco ("CODICE OK / APERTO") per **5 secondi**, dopodiché il sistema resetta le variabili e torna in Stand-by.
* **Innesco Allarme (Stato 6):** Se il codice inserito è errato, o se il timer di inserimento scade (raggiungimento dei 90 secondi senza aver completato il codice), il sistema porta ad `HIGH` il pin della sirena. L'allarme rimane attivo per un tempo prefissato (3000 tick da 100 ms), al termine del quale la sirena si spegne e il sistema si riarma tornando autonomamente in Stand-by.
* **Allontanamento e Sospensione (Stato 2):** Se durante la fase di inserimento l'utente si allontana oltre gli 80 cm, il sistema entra in uno stato di sospensione. Il timer di inserimento viene congelato. Se l'utente ritorna entro 80 cm, l'inserimento riprende esattamente da dove era stato interrotto. Se invece l'utente rimane lontano per più di **60 secondi consecutivi**, la sessione scade, lo schermo si pulisce e la centralina torna in Stand-by.

---

## 💻 Architettura Software e Vincoli Ingegneristici

Il firmware è stato sviluppato per garantire la massima reattività e la minima occupazione di memoria, sfruttando le caratteristiche hardware native dell'ATmega328P:

* **Manipolazione Diretta dei Registri:** La configurazione della direzione dei pin e la scrittura/lettura degli stati logici avvengono esclusivamente tramite operazioni bitwise sui registri `DDRx`, `PORTx` e `PINx`, eliminando l'overhead computazionale di funzioni come `digitalWrite()` o `digitalRead()`.
* **Cadenze Temporali Non Bloccanti (ISR):** Il codice rifiuta l'utilizzo di funzioni bloccanti. Il `TIMER1` del microcontrollore è configurato in modalità **CTC** (Clear Timer on Compare Match) con interrupt attivo ogni 10 microsecondi (`TIMER1_COMPA_vect`). Questo interrupt aggiorna un contatore di microsecondi globale, utilizzato nel `loop()` per gestire in modo asincrono la temporizzazione dell'impulso Trigger del sensore, il refresh dell'LCD (ogni 350 ms) e il decadimento di tutti i timer software (ogni 100 ms).
* **Gestione Asincrona del Sensore a Ultrasuoni:** Il segnale di Echo del sensore viene campionato confrontando lo stato attuale del pin con lo stato precedente memorizzato in una variabile statica, permettendo di calcolare il tempo di volo dell'onda sonora senza bloccare l'esecuzione del programma.

---

## 🔌 Setup Hardware e Pin Mapping

* **Display LCD 1602 (Interfaccia parallela a 8-bit):**
  * Controllo: Pin RS ➡️ `PORTB4` | Pin E ➡️ `PORTB3` (Pin R/W cablato direttamente a GND)
  * Dati D0-D5 ➡️ `PORTD2` fino a `PORTD7`
  * Dati D6-D7 ➡️ `PORTB2` e `PORTB5`
* **Sensore HC-SR04:**
  * Trigger ➡️ `PORTB0`
  * Echo ➡️ `PINB1`
* **Periferiche di Input/Output:**
  * Pulsante di Input ➡️ `PINC0` (Configurato con resistenza di pull-up interna attiva, attivo basso)
  * Sirena/Buzzer ➡️ `PORTC1`

## 🚀 Simulazione
Il file `diagram.json` incluso nel repository mappa accuratamente queste connessioni. Copiando il codice dello `sketch.ino` e il layout hardware sull'emulatore **Wokwi**, è possibile testare l'intero comportamento logico del sistema.
