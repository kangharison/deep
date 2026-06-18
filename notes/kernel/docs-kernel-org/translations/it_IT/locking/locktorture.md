# Funzionamento del testKernel Lock Torture

> 출처(원문): https://docs.kernel.org/translations/it_IT/locking/locktorture.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Warning

In caso di dubbi sulla correttezza del contenuto di questa traduzione,
l’unico riferimento valido è la documentazione ufficiale in inglese.
Per maggiori informazioni consultate le [avvertenze](../index.html#it-disclaimer).

# Funzionamento del test *Kernel Lock Torture*

## CONFIG\_LOCK\_TORTURE\_TEST

L’opzione di configurazione CONFIG\_LOCK\_TORTURE\_TEST fornisce un
modulo kernel che esegue delle verifiche che *torturano* le primitive di
sincronizzazione del kernel. Se dovesse servire, il modulo kernel,
‘locktorture’, può essere generato successivamente su un kernel che
volete verificare. Periodicamente le verifiche stampano messaggi tramite
`printk()` e che quindi possono essere letti tramite `dmesg` (magari
filtrate l’output con `grep "torture"`). La verifica inizia quando
il modulo viene caricato e termina quando viene rimosso. Questo
programma si basa sulle modalità di verifica di RCU tramite rcutorture.

Questa verifica consiste nella creazione di un certo numero di thread
del kernel che acquisiscono un blocco e lo trattengono per una certa
quantità di tempo così da simulare diversi comportamenti nelle sezioni
critiche. La quantità di contese su un blocco può essere simulata
allargando la sezione critica e/o creando più thread.

## Parametri del modulo

Questo modulo ha i seguenti parametri:

### Specifici di locktorture

nwriters\_stress
:   Numero di thread del kernel che stresseranno l’acquisizione
    esclusiva dei blocchi (scrittori). Il valore di base è il
    doppio del numero di processori attivi presenti.

nreaders\_stress
:   Numero di thread del kernel che stresseranno l’acquisizione
    condivisa dei blocchi (lettori). Il valore di base è lo stesso
    di nwriters\_stress. Se l’utente non ha specificato
    nwriters\_stress, allora entrambe i valori corrisponderanno
    al numero di processori attivi presenti.

torture\_type
:   Tipo di blocco da verificare. Di base, solo gli spinlock
    verranno verificati. Questo modulo può verificare anche
    i seguenti tipi di blocchi:

    > * “lock\_busted”:
    >   :   Simula un’incorretta implementazione del
    >       blocco.
    > * “spin\_lock”:
    >   :   coppie di `spin_lock()` e `spin_unlock()`.
    > * “spin\_lock\_irq”:
    >   :   coppie di `spin_lock_irq()` e `spin_unlock_irq()`.
    > * “rw\_lock”:
    >   :   coppie di rwlock read/write `lock()` e `unlock()`.
    > * “rw\_lock\_irq”:
    >   :   copie di rwlock read/write `lock_irq()` e
    >       `unlock_irq()`.
    > * “mutex\_lock”:
    >   :   coppie di [`mutex_lock()`](../../../kernel-hacking/locking.html#c.mutex_lock "mutex_lock") e [`mutex_unlock()`](../../../kernel-hacking/locking.html#c.mutex_unlock "mutex_unlock").
    > * “rtmutex\_lock”:
    >   :   coppie di `rtmutex_lock()` e `rtmutex_unlock()`.
    >       Il kernel deve avere CONFIG\_RT\_MUTEXES=y.
    > * “rwsem\_lock”:
    >   :   coppie di semafori read/write `down()` e `up()`.

### Generici dell’ambiente di sviluppo ‘torture’ (RCU + locking)

shutdown\_secs
:   Numero di secondi prima che la verifica termini e il sistema
    venga spento. Il valore di base è zero, il che disabilita
    la possibilità di terminare e spegnere. Questa funzionalità
    può essere utile per verifiche automatizzate.

onoff\_interval
:   Numero di secondi fra ogni tentativo di esecuzione di
    un’operazione casuale di CPU-hotplug. Di base è zero, il
    che disabilita la funzionalità di CPU-hotplug. Nei kernel
    con CONFIG\_HOTPLUG\_CPU=n, locktorture si rifiuterà, senza
    dirlo, di effettuare una qualsiasi operazione di
    CPU-hotplug indipendentemente dal valore specificato in
    onoff\_interval.

onoff\_holdoff
:   Numero di secondi da aspettare prima di iniziare le
    operazioni di CPU-hotplug. Normalmente questo verrebbe
    usato solamente quando locktorture è compilato come parte
    integrante del kernel ed eseguito automaticamente all’avvio,
    in questo caso è utile perché permette di non confondere
    l’avvio con i processori che vanno e vengono. Questo
    parametro è utile sono se CONFIG\_HOTPLUG\_CPU è abilitato.

stat\_interval
:   Numero di secondi fra una stampa ([`printk()`](../../../core-api/printk-basics.html#c.printk "printk")) delle
    statistiche e l’altra. Di base, locktorture riporta le
    statistiche ogni 60 secondi. Impostando l’intervallo a 0
    ha l’effetto di stampare le statistiche -solo- quando il
    modulo viene rimosso.

stutter
:   Durata della verifica prima di effettuare una pausa di
    eguale durata. Di base “stutter=5”, quindi si eseguono
    verifiche e pause di (circa) cinque secondi.
    L’impostazione di “stutter=0” fa si che la verifica
    venga eseguita continuamente senza fermarsi.

shuffle\_interval
:   Il numero di secondi per cui un thread debba mantenere
    l’affinità con un sottoinsieme di processori, di base è
    3 secondi. Viene usato assieme a test\_no\_idle\_hz.

verbose
:   Abilita le stampe di debug, via [`printk()`](../../../core-api/printk-basics.html#c.printk "printk"). Di base è
    abilitato. Queste informazioni aggiuntive sono per la
    maggior parte relative ad errori di alto livello e resoconti
    da parte dell’struttura ‘torture’.

## Statistiche

Le statistiche vengono stampate secondo il seguente formato:

```
spin_lock-torture: Writes:  Total: 93746064  Max/Min: 0/0   Fail: 0
   (A)                    (B)            (C)            (D)          (E)

(A): tipo di lock sotto verifica -- parametro torture_type.

(B): Numero di acquisizione del blocco in scrittura. Se si ha a che fare
     con una primitiva di lettura/scrittura apparirà di seguito anche una
     seconda voce "Reads"

(C): Numero di volte che il blocco è stato acquisito

(D): Numero minimo e massimo di volte che un thread ha fallito
     nell'acquisire il blocco

(E): valori true/false nel caso di errori durante l'acquisizione del blocco.
     Questo dovrebbe dare un riscontro positivo -solo- se c'è un baco
     nell'implementazione delle primitive di sincronizzazione. Altrimenti un
     blocco non dovrebbe mai fallire (per esempio, spin_lock()).
     Ovviamente lo stesso si applica per (C). Un semplice esempio è il tipo
     "lock_busted".
```

## Uso

Il seguente script può essere utilizzato per verificare i blocchi:

```
#!/bin/sh

modprobe locktorture
sleep 3600
rmmod locktorture
dmesg | grep torture:
```

L’output può essere manualmente ispezionato cercando il marcatore d’errore
“!!!”. Ovviamente potreste voler creare degli script più elaborati che
verificano automaticamente la presenza di errori. Il comando “rmmod” forza la
stampa (usando [`printk()`](../../../core-api/printk-basics.html#c.printk "printk")) di “SUCCESS”, “FAILURE”, oppure “RCU\_HOTPLUG”. I primi
due si piegano da soli, mentre l’ultimo indica che non stati trovati problemi di
sincronizzazione, tuttavia ne sono stati trovati in CPU-hotplug.

Consultate anche: [Le operazioni RCU per le verifiche torture](../RCU/torture.html)
