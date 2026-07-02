int Stato = 0; 
int NumeroAttuale = 0;
int CifreInserite = 0;
int CodiceSegreto[5] = {1, 2, 3, 4, 5}; //Modificabile
int CodiceUtente[5];
bool Presenza = false;    
volatile unsigned long microsecondi = 0;
int TimerInserimento = 0;      // Timer1
int TimerAllontanamento = 0;    // Timer2
int TimerAllarme = 0;          // Timer300
int TimerAccesso = 0;          // Timer feedback

unsigned long ultimoAggiornamentoTimer = 0;
unsigned long tempoUltimaLettura = 0;       
unsigned long tempoUltimoAggiornamentoLCD = 0; 
unsigned long inizioEcho = 0;
unsigned long durataEcho = 0;
bool attesaEcho = false;
int distanzaRilevata = 999;

ISR(TIMER1_COMPA_vect) {        // Gestione timer
    microsecondi += 10; //Incrementa il contatore 10 microsecondi ogni attivazione dell'interruzione 
}


void inviaComandoLcd(uint8_t dato, uint8_t tipo) {
    if (tipo == 1) {            //Se il tipo è un carattere
        PORTB |= (1 << 4);      //imposto a 1 (HIGH) il bit 4 di PORTB (Pin RS)
    } else {                    //altrimenti
        PORTB &= ~(1 << 4);     //imposto a 0 (LOW) il bit 4 di PORTB (Pin RS)
    }

    if (dato & 1) {             //Verifico se il bit 0 del byte "dato" è pari a 1
        PORTD |= (1 << 2);      //Imposto HIGH il bit 2 di PORTD (invia il segnale al pin D0 dell'LCD)
    } else {                    //altrimenti
        PORTD &= ~(1 << 2);     //Imposto a LOW il bit 2 di PORTD (spegne il segnale sul pin D0 dell'LCD)
    }

    if (dato & 2) {             //Verifico il bit 1
        PORTD |= (1 << 3);      //Imposto HIGH il bit 3 di PORTD (pin D1 LCD)
    } else {
        PORTD &= ~(1 << 3);     //Imposto LOW il bit 3 di PORTD (pin D1 LCD)
    }

    if (dato & 4) {             //Verifico il bit 2
        PORTD |= (1 << 4);      //pin D2 LCD
    } else {
        PORTD &= ~(1 << 4);     //pin D2 LCD
    }

    if (dato & 8) {             //Verifico il bit 3
        PORTD |= (1 << 5);      //pin D3 LCD
    } else {
        PORTD &= ~(1 << 5);     //pin D3 LCD
    }

    if (dato & 16) {            //Verifico il bit 4
        PORTD |= (1 << 6);      //pin D4 LCD
    } else {
        PORTD &= ~(1 << 6);     //pin D4 LCD
    }

    if (dato & 32) {            //Verifico il bit 5
        PORTD |= (1 << 7);      //pin D5 LCD
    } else {
        PORTD &= ~(1 << 7);     //pin D5 LCD
    }

    if (dato & 64) {            //Verifico il bit 6
        PORTB |= (1 << 2);      //pin D6 LCD
    } else {
        PORTB &= ~(1 << 2);     //pin D6 LCD
    }

    if (dato & 128) {           //Verifico il bit 7
        PORTB |= (1 << 5);      //pin D7 LCD
    } else {
        PORTB &= ~(1 << 5);     //pin D7 LCD
    }

    PORTB |= (1 << 3);                  //Imposto a 1 il bit 3 di PORTB (E dell'LCD)
    for(volatile int i=0; i<30; i++);   // microritardo
    PORTB &= ~(1 << 3);                 //Riporto LOW il bit 3 di PORTB
    for(volatile int i=0; i<150; i++);  // microritardo
}

void setup() {
    cli();          //Disabilito gli interrupt
    TCCR1A = 0;     //Azzero il registro di controllo A del Timer 1
    TCCR1B = 0;     //Azzero il registro di controllo B del Timer 1
    OCR1A = 159;    //Imposto il valore di confronto del Timer 1 per generare un interrupt a intervalli regolari
    TCCR1B |= (1 << WGM12) | (1 << CS10);   //Configuro il Timer 1 in modalità CTC con nessun prescaler (CS10)
    TIMSK1 |= (1 << OCIE1A);                // Abilito interrupt del Timer 1
    sei();          //Riabilito gli interrupt

    DDRD |= 0xFC;       //Configuro pin da 2 a 7 di PORTD come uscite digitali (DDRD |= 11111100 in binario)
    DDRB |= 0x3D;       //Configuro i pin 0, 2, 3, 4, 5 di PORTB come uscite digitali
    DDRB &= ~(1 << 1);  //Configuro pin 1 di PORTB come i. d. (pin di Echo del sensore)
    DDRC |= (1 << 1);   //Configuro pin 1 di PORTC come uscita digitale (controllo della sirena/allarme)
    DDRC &= ~(1 << 0);  //Configuro pin 0 di PORTC come i. d. (collegamento del pulsante)
    PORTC |= (1 << 0);  //Attivo la resistenza di pull-up interna sul pin 0 di PORTC (HIGH)

    inviaComandoLcd(0x38, 0); //Inizializzazione LCD per la modalità a 8-bit
    inviaComandoLcd(0x0C, 0); //Invio il comando per accendere il display 
    inviaComandoLcd(0x01, 0); //Ripulisco completamente lo schermo e resetto il cursore
}

void loop() {


    if (microsecondi - tempoUltimaLettura > 100000) {   //Attendo 100 millisecondi 
        PORTB |= (1 << 0);                  //Alzo il bit 0 di PORTB per attivare il pin Trigger del sensore a ultrasuoni
        for(volatile int i=0; i<30; i++);   // microritardo
        PORTB &= ~(1 << 0);                 //AbbassO il bit 0 di PORTB terminando l'impulso di Trigger ed emettendo l'onda sonora
        tempoUltimaLettura = microsecondi;  // aggiorno
        attesaEcho = true;                  //Imposto lo stato logico in attesa dell'onda di ritorno (Echo)
    }

    static bool statoEchoPrecedente = false;    //Stato pin echo precedente
    bool statoEchoAttuale = (PINB & (1 << 1));  //Attuale echo

    if (attesaEcho) {     //Se il sistema ha precedentemente inviato un impulso di Trigger
        if (!statoEchoPrecedente && statoEchoAttuale) { //Se lo stato dell'echo è cambiato(Onda partita)
            inizioEcho = microsecondi;    //Salvo inizio echo
        }
        if (statoEchoPrecedente && !statoEchoAttuale) { //Se lo stato dell'echo è cambiato(Onda tornata)
            durataEcho = microsecondi - inizioEcho; //Calcolo la differenza
            distanzaRilevata = durataEcho * 0.017;  //La converto in centimetri
            Presenza = (distanzaRilevata > 0 && distanzaRilevata < 80); //True se <80 centimetri
            attesaEcho = false;   //Spenge il flag di attesa fino al prossimo invio di segnale
        }
    }
    statoEchoPrecedente = statoEchoAttuale;  //Aggiorno stato echo

    if (microsecondi - ultimoAggiornamentoTimer > 100000) { //Ogni 0.1s diminuisco tutti i timer attivi di 1
        if (TimerInserimento > 0) {
            TimerInserimento--; 
        }
        if (TimerAllontanamento > 0) {
            TimerAllontanamento--; 
        }
        if (TimerAllarme > 0) {
            TimerAllarme--;
        }
        if (TimerAccesso > 0) {
            TimerAccesso--;
        }
        ultimoAggiornamentoTimer = microsecondi;
    }

    if (microsecondi - tempoUltimoAggiornamentoLCD > 350000) {  //Aggiorno lo schermo ogni 0.35s
        inviaComandoLcd(0x80, 0);           //Sposta il cursore LCD all'inizio della prima riga (indirizzo memoria 0x80)
        switch(Stato) {                   // Seleziono cosa scrivere sullo schermo analizzando il valore della variabile di Stato
            case 0:                       // STAND-BY
                inviaComandoLcd('S',1);   //Scrivo la stringa carattere per carattere sulla prima riga
                inviaComandoLcd('I',1);   
                inviaComandoLcd('S',1);   
                inviaComandoLcd('T',1);   
                inviaComandoLcd('E',1);   
                inviaComandoLcd('M',1);   
                inviaComandoLcd('A',1);   
                inviaComandoLcd(' ',1); 
                inviaComandoLcd('I',1); 
                inviaComandoLcd('N',1);
                inviaComandoLcd(0xC0, 0); //Sposto il cursore all'inizio della seconda  riga
                inviaComandoLcd('S',1);   //Scrivo carattere per carattere
                inviaComandoLcd('T',1); 
                inviaComandoLcd('A',1); 
                inviaComandoLcd('N',1); 
                inviaComandoLcd('D',1); 
                inviaComandoLcd('-',1); 
                inviaComandoLcd('B',1); 
                inviaComandoLcd('Y',1);
                break;  //Esce

            case 1: //Entro nel 
            case 2: // case 3
            case 3: // INSERIMENTO / SOSPESO / CONFERMA
                inviaComandoLcd('I',1); 
                inviaComandoLcd('N',1); 
                inviaComandoLcd('S',1); 
                inviaComandoLcd('E',1); 
                inviaComandoLcd('R',1); 
                inviaComandoLcd('I',1); 
                inviaComandoLcd('S',1); 
                inviaComandoLcd('C',1); 
                inviaComandoLcd('I',1); 
                inviaComandoLcd(' ',1); 
                inviaComandoLcd('C',1); 
                inviaComandoLcd('O',1); 
                inviaComandoLcd('D',1); 
                inviaComandoLcd('I',1); 
                inviaComandoLcd('C',1); 
                inviaComandoLcd('E',1);
                inviaComandoLcd(0xC0, 0);
                if (Stato == 2) { 
                    inviaComandoLcd('S',1); 
                    inviaComandoLcd('O',1); 
                    inviaComandoLcd('S',1); 
                    inviaComandoLcd('P',1); 
                    inviaComandoLcd('E',1); 
                    inviaComandoLcd('S',1); 
                    inviaComandoLcd('O',1);
                } else {
                    for(int i=0; i<CifreInserite; i++) {      //Inserisco tanti * quante cifre inserite
                        inviaComandoLcd('*', 1);
                    }
                    inviaComandoLcd(NumeroAttuale + '0', 1);   //Cifra del codice attuale
                    for(int i=0; i<(4-CifreInserite); i++) {  //Inserisco tanti _ quante cifre da inserire
                        inviaComandoLcd('_', 1); 
                    }
                }
                for(int i=0; i<10; i++) {       //Pulisco il resto dell'LCD con spazi vuoti
                    inviaComandoLcd(' ', 1);
                }
                break;

            case 5: // ACCESSO
                inviaComandoLcd('C',1); 
                inviaComandoLcd('O',1); 
                inviaComandoLcd('D',1); 
                inviaComandoLcd('I',1); 
                inviaComandoLcd('C',1); 
                inviaComandoLcd('E',1); 
                inviaComandoLcd(' ',1);
                inviaComandoLcd('O',1); 
                inviaComandoLcd('K',1);
                inviaComandoLcd(0xC0, 0);
                inviaComandoLcd('A',1); 
                inviaComandoLcd('P',1); 
                inviaComandoLcd('E',1); 
                inviaComandoLcd('R',1); 
                inviaComandoLcd('T',1); 
                inviaComandoLcd('O',1);
                break;

            case 6: // ALLARME
                inviaComandoLcd('!',1); 
                inviaComandoLcd('A',1); 
                inviaComandoLcd('L',1); 
                inviaComandoLcd('L',1); 
                inviaComandoLcd('A',1); 
                inviaComandoLcd('R',1); 
                inviaComandoLcd('M',1); 
                inviaComandoLcd('E',1); 
                inviaComandoLcd('!',1);
                inviaComandoLcd(0xC0, 0);
                inviaComandoLcd('I',1); 
                inviaComandoLcd('N',1); 
                inviaComandoLcd('T',1); 
                inviaComandoLcd('R',1); 
                inviaComandoLcd('U',1); 
                inviaComandoLcd('S',1); 
                inviaComandoLcd('O',1);
                break;
        }
        tempoUltimoAggiornamentoLCD = microsecondi;
    }

    switch (Stato) {
        case 0:   // STAND-BY
            PORTC &= ~(1 << 1);   //Spegne l'allarme/sirena azzerando il bit 1 di PORTC
            if (distanzaRilevata > 0 && distanzaRilevata < 80) { //Se distanza<80cm
                Stato = 1; 
                TimerInserimento = 900; //Sarebbe il 90 però togliamo 1 ogni 0.1s
                CifreInserite = 0; 
                NumeroAttuale = 0;
                inviaComandoLcd(0x01, 0); //Puliamo lo schermo
            }
            break;

        case 1: // INSERIMENTO
            if (TimerInserimento == 0) { //Dopo 90 secondi
                Stato = 6;  //Allarme
                TimerAllarme = 3000; 
                break; 
            }
            if (!Presenza) { //Se la distanza supera gli 80cm
                Stato = 2; 
                TimerAllontanamento = 600; //Avviamo il timer2
            } else {
                TimerAllontanamento = 0;
                static bool pulsanteVecchio = true; //Posizione del pulsante precedente
                bool pulsanteAttuale = (PINC & (1 << 0)); //Attuale
                static unsigned long tempoPressione = 0;    
                if (pulsanteVecchio && !pulsanteAttuale) {  //Se il pulsante viene premuto
                    tempoPressione = microsecondi;
                }
                if (!pulsanteVecchio && pulsanteAttuale) {  //Quando viene lasciato
                    unsigned long durata = microsecondi - tempoPressione;//Calcoliamo il tempo di pressione
                    if (durata > 1000000) { //Se supeera 1 secondo
                        Stato = 3; 
                    } else {  //Se non lo supera
                        NumeroAttuale = (NumeroAttuale + 1) % 10;//Cambio la cifra nell'LCD
                    }
                }
                pulsanteVecchio = pulsanteAttuale;
            }
            break;

        case 2: // SOSPESO
            if (Presenza) { //Se distanza<80cm
                Stato = 1; 
            }
            if (TimerAllontanamento == 0) { //Dopo 60 secondi
                Stato = 0; 
                inviaComandoLcd(0x01, 0); //Pulisci schermo
            }
            break;

        case 3: // CONFERMA_CIFRA
            CodiceUtente[CifreInserite] = NumeroAttuale;  //Memorizza la cifra inserita nella posizione attuale
            CifreInserite++; //Aumenta la posizione
            NumeroAttuale = 0;  //Reimposta la cifra  a 0
            if (CifreInserite == 5) { //Quando sono state inserite 5 cifre
                Stato = 4; 
            } else {
                Stato = 1;
            }
            break;

        case 4: // CONTROLLO
            {
                bool corretto = true;
                for(int i=0; i<5; i++) {
                    if(CodiceUtente[i] != CodiceSegreto[i]) { //Se nellee 5 posizioni il codice diverge dal codicesegreto
                        corretto = false;
                    }
                }
                if(corretto) { //Se il codice è corretto
                    Stato = 5; 
                    TimerAccesso = 50; //5 secondi per visualizzare il messaggio
                } else { 
                    Stato = 6; 
                    TimerAllarme = 3000; //Avvio timer allarme
                }
                inviaComandoLcd(0x01, 0);//Pulisci schermo
            }
            break;

        case 5: // ACCESSO
            if (TimerAccesso == 0) { 
                Stato = 0; 
                inviaComandoLcd(0x01, 0); 
            }
            break;

        case 6: // ALLARME
            PORTC |= (1 << 1); 
            if (TimerAllarme == 0) { 
                Stato = 0; 
                inviaComandoLcd(0x01, 0); 
            }
            break;
    }
}
