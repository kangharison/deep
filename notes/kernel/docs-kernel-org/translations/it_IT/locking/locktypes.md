# Tipologie di blocco e le loro istruzioni

> 출처(원문): https://docs.kernel.org/translations/it_IT/locking/locktypes.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Warning

In caso di dubbi sulla correttezza del contenuto di questa traduzione,
l’unico riferimento valido è la documentazione ufficiale in inglese.
Per maggiori informazioni consultate le [avvertenze](../index.html#it-disclaimer).

# Tipologie di blocco e le loro istruzioni

## Introduzione

Il kernel fornisce un certo numero di primitive di blocco che possiamo dividere
in tre categorie:

> * blocchi ad attesa con sospensione
> * blocchi locali per CPU
> * blocchi ad attesa attiva

Questo documento descrive questi tre tipi e fornisce istruzioni su come
annidarli, ed usarli su kernel PREEMPT\_RT.

## Categorie di blocchi

### Blocchi ad attesa con sospensione

I blocchi ad attesa con sospensione possono essere acquisiti solo in un contesti
dov’è possibile la prelazione.

Diverse implementazioni permettono di usare `try_lock()` anche in altri contesti,
nonostante ciò è bene considerare anche la sicurezza dei corrispondenti
`unlock()`. Inoltre, vanno prese in considerazione anche le varianti di *debug*
di queste primitive. Insomma, non usate i blocchi ad attesa con sospensioni in
altri contesti a meno che proprio non vi siano alternative.

In questa categoria troviamo:

> * mutex
> * rt\_mutex
> * semaphore
> * rw\_semaphore
> * ww\_mutex
> * percpu\_rw\_semaphore

Nei kernel con PREEMPT\_RT, i seguenti blocchi sono convertiti in blocchi ad
attesa con sospensione:

> * local\_lock
> * spinlock\_t
> * rwlock\_t

### Blocchi locali per CPU

> * local\_lock

Su kernel non-PREEMPT\_RT, le funzioni local\_lock gestiscono le primitive di
disabilitazione di prelazione ed interruzioni. Al contrario di altri meccanismi,
la disabilitazione della prelazione o delle interruzioni sono puri meccanismi
per il controllo della concorrenza su una CPU e quindi non sono adatti per la
gestione della concorrenza inter-CPU.

### Blocchi ad attesa attiva

> * raw\_spinlcok\_t
> * bit spinlocks
>
> Nei kernel non-PREEMPT\_RT, i seguenti blocchi sono ad attesa attiva:
>
> * spinlock\_t
> * rwlock\_t

Implicitamente, i blocchi ad attesa attiva disabilitano la prelazione e le
funzioni lock/unlock hanno anche dei suffissi per gestire il livello di
protezione:

> |  |  |
> | --- | --- |
> | `_bh()` | disabilita / abilita *bottom halves* (interruzioni software) |
> | `_irq()` | disabilita / abilita le interruzioni |
> | \_irqsave/`restore()` | salva e disabilita le interruzioni / ripristina ed attiva le interruzioni |

## Semantica del proprietario

Eccetto i semafori, i sopracitati tipi di blocchi hanno tutti una semantica
molto stringente riguardo al proprietario di un blocco:

> Il contesto (attività) che ha acquisito il blocco deve rilasciarlo

I semafori rw\_semaphores hanno un’interfaccia speciale che permette anche ai non
proprietari del blocco di rilasciarlo per i lettori.

## rtmutex

I blocchi a mutua esclusione RT (*rtmutex*) sono un sistema a mutua esclusione
con supporto all’ereditarietà della priorità (PI).

Questo meccanismo ha delle limitazioni sui kernel non-PREEMPT\_RT dovuti alla
prelazione e alle sezioni con interruzioni disabilitate.

Chiaramente, questo meccanismo non può avvalersi della prelazione su una sezione
dove la prelazione o le interruzioni sono disabilitate; anche sui kernel
PREEMPT\_RT. Tuttavia, i kernel PREEMPT\_RT eseguono la maggior parte delle
sezioni in contesti dov’è possibile la prelazione, specialmente in contesti
d’interruzione (anche software). Questa conversione permette a spinlock\_t e
rwlock\_t di essere implementati usando rtmutex.

## semaphore

La primitiva semaphore implementa un semaforo con contatore.

I semafori vengono spesso utilizzati per la serializzazione e l’attesa, ma per
nuovi casi d’uso si dovrebbero usare meccanismi diversi, come mutex e
completion.

### semaphore e PREEMPT\_RT

I kernel PREEMPT\_RT non cambiano l’implementazione di semaphore perché non hanno
un concetto di proprietario, dunque impediscono a PREEMPT\_RT d’avere
l’ereditarietà della priorità sui semafori. Un proprietario sconosciuto non può
ottenere una priorità superiore. Di consequenza, bloccarsi sui semafori porta
all’inversione di priorità.

## rw\_semaphore

Il blocco rw\_semaphore è un meccanismo che permette più lettori ma un solo scrittore.

Sui kernel non-PREEMPT\_RT l’implementazione è imparziale, quindi previene
l’inedia dei processi scrittori.

Questi blocchi hanno una semantica molto stringente riguardo il proprietario, ma
offre anche interfacce speciali che permettono ai processi non proprietari di
rilasciare un processo lettore. Queste interfacce funzionano indipendentemente
dalla configurazione del kernel.

### rw\_semaphore e PREEMPT\_RT

I kernel PREEMPT\_RT sostituiscono i rw\_semaphore con un’implementazione basata
su rt\_mutex, e questo ne modifica l’imparzialità:

> Dato che uno scrittore rw\_semaphore non può assicurare la propria priorità ai
> suoi lettori, un lettore con priorità più bassa che ha subito la prelazione
> continuerà a trattenere il blocco, quindi porta all’inedia anche gli scrittori
> con priorità più alta. Per contro, dato che i lettori possono garantire la
> propria priorità agli scrittori, uno scrittore a bassa priorità che subisce la
> prelazione vedrà la propria priorità alzata finché non rilascerà il blocco, e
> questo preverrà l’inedia dei processi lettori a causa di uno scrittore.

## local\_lock

I local\_lock forniscono nomi agli ambiti di visibilità delle sezioni critiche
protette tramite la disattivazione della prelazione o delle interruzioni.

Sui kernel non-PREEMPT\_RT le operazioni local\_lock si traducono
nell’abilitazione o disabilitazione della prelazione o le interruzioni.

> |  |  |
> | --- | --- |
> | local\_lock(&llock) | `preempt_disable()` |
> | local\_unlock(&llock) | `preempt_enable()` |
> | local\_lock\_irq(&llock) | `local_irq_disable()` |
> | local\_unlock\_irq(&llock) | `local_irq_enable()` |
> | local\_lock\_irqsave(&llock) | `local_irq_save()` |
> | local\_unlock\_irqrestore(&llock) | `local_irq_restore()` |

Gli ambiti di visibilità con nome hanno due vantaggi rispetto alle primitive di
base:

> * Il nome del blocco permette di fare un’analisi statica, ed è anche chiaro su
>   cosa si applichi la protezione cosa che invece non si può fare con le
>   classiche primitive in quanto sono opache e senza alcun ambito di
>   visibilità.
> * Se viene abilitato lockdep, allora local\_lock ottiene un lockmap che
>   permette di verificare la bontà della protezione. Per esempio, questo può
>   identificare i casi dove una funzione usa `preempt_disable()` come meccanismo
>   di protezione in un contesto d’interruzione (anche software). A parte
>   questo, lockdep\_assert\_held(&llock) funziona come tutte le altre primitive
>   di sincronizzazione.

### local\_lock e PREEMPT\_RT

I kernel PREEMPT\_RT sostituiscono local\_lock con uno spinlock\_t per CPU, quindi
ne cambia la semantica:

> * Tutte le modifiche a spinlock\_t si applicano anche a local\_lock

### L’uso di local\_lock

I local\_lock dovrebbero essere usati su kernel non-PREEMPT\_RT quando la
disabilitazione della prelazione o delle interruzioni è il modo più adeguato per
gestire l’accesso concorrente a strutture dati per CPU.

Questo meccanismo non è adatto alla protezione da prelazione o interruzione su
kernel PREEMPT\_RT dato che verrà convertito in spinlock\_t.

## raw\_spinlock\_t e spinlock\_t

### raw\_spinlock\_t

I blocco raw\_spinlock\_t è un blocco ad attesa attiva su tutti i tipi di kernel,
incluso quello PREEMPT\_RT. Usate raw\_spinlock\_t solo in sezioni critiche nel
cuore del codice, nella gestione delle interruzioni di basso livello, e in posti
dove è necessario disabilitare la prelazione o le interruzioni. Per esempio, per
accedere in modo sicuro lo stato dell’hardware. A volte, i raw\_spinlock\_t
possono essere usati quando la sezione critica è minuscola, per evitare gli
eccessi di un rtmutex.

### spinlock\_t

Il significato di spinlock\_t cambia in base allo stato di PREEMPT\_RT.

Sui kernel non-PREEMPT\_RT, spinlock\_t si traduce in un raw\_spinlock\_t ed ha
esattamente lo stesso significato.

### spinlock\_t e PREEMPT\_RT

Sui kernel PREEMPT\_RT, spinlock\_t ha un’implementazione dedicata che si basa
sull’uso di rt\_mutex. Questo ne modifica il significato:

> * La prelazione non viene disabilitata.
> * I suffissi relativi alla interruzioni (\_irq, \_irqsave / \_irqrestore) per le
>   operazioni spin\_lock / spin\_unlock non hanno alcun effetto sullo stato delle
>   interruzioni della CPU.
> * I suffissi relativi alle interruzioni software (`_bh()`) disabilitano i
>   relativi gestori d’interruzione.
>
>   I kernel non-PREEMPT\_RT disabilitano la prelazione per ottenere lo stesso effetto.
>
>   I kernel PREEMPT\_RT usano un blocco per CPU per la serializzazione, il che
>   permette di tenere attiva la prelazione. Il blocco disabilita i gestori
>   d’interruzione software e previene la rientranza vista la prelazione attiva.

A parte quanto appena discusso, i kernel PREEMPT\_RT preservano il significato
di tutti gli altri aspetti di spinlock\_t:

> * Le attività che trattengono un blocco spinlock\_t non migrano su altri
>   processori. Disabilitando la prelazione, i kernel non-PREEMPT\_RT evitano la
>   migrazione. Invece, i kernel PREEMPT\_RT disabilitano la migrazione per
>   assicurarsi che i puntatori a variabili per CPU rimangano validi anche
>   quando un’attività subisce la prelazione.
> * Lo stato di un’attività si mantiene durante le acquisizioni del blocco al
>   fine di garantire che le regole basate sullo stato delle attività si possano
>   applicare a tutte le configurazioni del kernel. I kernel non-PREEMPT\_RT
>   lasciano lo stato immutato. Tuttavia, la funzionalità PREEMPT\_RT deve
>   cambiare lo stato se l’attività si blocca durante l’acquisizione. Dunque,
>   salva lo stato attuale prima di bloccarsi ed il rispettivo risveglio lo
>   ripristinerà come nell’esempio seguente:
>
>   ```
>   task->state = TASK_INTERRUPTIBLE
>    lock()
>      block()
>        task->saved_state = task->state
>        task->state = TASK_UNINTERRUPTIBLE
>        schedule()
>                                       lock wakeup
>                                         task->state = task->saved_state
>   ```
>
>   Altri tipi di risvegli avrebbero impostato direttamente lo stato a RUNNING,
>   ma in questo caso non avrebbe funzionato perché l’attività deve rimanere
>   bloccata fintanto che il blocco viene trattenuto. Quindi, lo stato salvato
>   viene messo a RUNNING quando il risveglio di un non-blocco cerca di
>   risvegliare un’attività bloccata in attesa del rilascio di uno spinlock. Poi,
>   quando viene completata l’acquisizione del blocco, il suo risveglio
>   ripristinerà lo stato salvato, in questo caso a RUNNING:
>
>   ```
>   task->state = TASK_INTERRUPTIBLE
>    lock()
>      block()
>        task->saved_state = task->state
>        task->state = TASK_UNINTERRUPTIBLE
>        schedule()
>                                       non lock wakeup
>                                         task->saved_state = TASK_RUNNING
>
>                                       lock wakeup
>                                         task->state = task->saved_state
>   ```
>
>   Questo garantisce che il vero risveglio non venga perso.

## rwlock\_t

Il blocco rwlock\_t è un meccanismo che permette più lettori ma un solo scrittore.

Sui kernel non-PREEMPT\_RT questo è un blocco ad attesa e per i suoi suffissi si
applicano le stesse regole per spinlock\_t. La sua implementazione è imparziale,
quindi previene l’inedia dei processi scrittori.

### rwlock\_t e PREEMPT\_RT

Sui kernel PREEMPT\_RT rwlock\_t ha un’implementazione dedicata che si basa
sull’uso di rt\_mutex. Questo ne modifica il significato:

> * Tutte le modifiche fatte a spinlock\_t si applicano anche a rwlock\_t.
> * Dato che uno scrittore rw\_semaphore non può assicurare la propria priorità ai
>   suoi lettori, un lettore con priorità più bassa che ha subito la prelazione
>   continuerà a trattenere il blocco, quindi porta all’inedia anche gli
>   scrittori con priorità più alta. Per contro, dato che i lettori possono
>   garantire la propria priorità agli scrittori, uno scrittore a bassa priorità
>   che subisce la prelazione vedrà la propria priorità alzata finché non
>   rilascerà il blocco, e questo preverrà l’inedia dei processi lettori a causa
>   di uno scrittore.

## Precisazioni su PREEMPT\_RT

### local\_lock su RT

Sui kernel PREEMPT\_RT Ci sono alcune implicazioni dovute alla conversione di
local\_lock in un spinlock\_t. Per esempio, su un kernel non-PREEMPT\_RT il
seguente codice funzionerà come ci si aspetta:

```
local_lock_irq(&local_lock);
raw_spin_lock(&lock);
```

ed è equivalente a:

```
raw_spin_lock_irq(&lock);
```

Ma su un kernel PREEMPT\_RT questo codice non funzionerà perché `local_lock_irq()`
si traduce in uno spinlock\_t per CPU che non disabilita né le interruzioni né la
prelazione. Il seguente codice funzionerà su entrambe i kernel con o senza
PREEMPT\_RT:

```
local_lock_irq(&local_lock);
spin_lock(&lock);
```

Un altro dettaglio da tenere a mente con local\_lock è che ognuno di loro ha un
ambito di protezione ben preciso. Dunque, la seguente sostituzione è errate:

```
func1()
{
  local_irq_save(flags);    -> local_lock_irqsave(&local_lock_1, flags);
  func3();
  local_irq_restore(flags); -> local_unlock_irqrestore(&local_lock_1, flags);
}

func2()
{
  local_irq_save(flags);    -> local_lock_irqsave(&local_lock_2, flags);
  func3();
  local_irq_restore(flags); -> local_unlock_irqrestore(&local_lock_2, flags);
}

func3()
{
  lockdep_assert_irqs_disabled();
  access_protected_data();
}
```

Questo funziona correttamente su un kernel non-PREEMPT\_RT, ma su un kernel
PREEMPT\_RT local\_lock\_1 e local\_lock\_2 sono distinti e non possono serializzare
i chiamanti di `func3()`. L’*assert* di lockdep verrà attivato su un kernel
PREEMPT\_RT perché `local_lock_irqsave()` non disabilita le interruzione a casa
della specifica semantica di spinlock\_t in PREEMPT\_RT. La corretta sostituzione
è:

```
func1()
{
  local_irq_save(flags);    -> local_lock_irqsave(&local_lock, flags);
  func3();
  local_irq_restore(flags); -> local_unlock_irqrestore(&local_lock, flags);
}

func2()
{
  local_irq_save(flags);    -> local_lock_irqsave(&local_lock, flags);
  func3();
  local_irq_restore(flags); -> local_unlock_irqrestore(&local_lock, flags);
}

func3()
{
  lockdep_assert_held(&local_lock);
  access_protected_data();
}
```

### spinlock\_t e rwlock\_t

Ci sono alcune conseguenze di cui tener conto dal cambiamento di semantica di
spinlock\_t e rwlock\_t sui kernel PREEMPT\_RT. Per esempio, sui kernel non
PREEMPT\_RT il seguente codice funziona come ci si aspetta:

```
local_irq_disable();
spin_lock(&lock);
```

ed è equivalente a:

```
spin_lock_irq(&lock);
```

Lo stesso vale per rwlock\_t e le varianti con `_irqsave()`.

Sui kernel PREEMPT\_RT questo codice non funzionerà perché gli rtmutex richiedono
un contesto con la possibilità di prelazione. Al suo posto, usate
`spin_lock_irq()` o `spin_lock_irqsave()` e le loro controparti per il rilascio. I
kernel PREEMPT\_RT offrono un meccanismo local\_lock per i casi in cui la
disabilitazione delle interruzioni ed acquisizione di un blocco devono rimanere
separati. Acquisire un local\_lock àncora un processo ad una CPU permettendo cose
come un’acquisizione di un blocco con interruzioni disabilitate per singola CPU.

Il tipico scenario è quando si vuole proteggere una variabile di processore nel
contesto di un thread:

```
struct foo *p = get_cpu_ptr(&var1);

spin_lock(&p->lock);
p->count += this_cpu_read(var2);
```

Questo codice è corretto su un kernel non-PREEMPT\_RT, ma non lo è su un
PREEMPT\_RT. La modifica della semantica di spinlock\_t su PREEMPT\_RT non permette
di acquisire p->lock perché, implicitamente, `get_cpu_ptr()` disabilita la
prelazione. La seguente sostituzione funzionerà su entrambe i kernel:

```
struct foo *p;

migrate_disable();
p = this_cpu_ptr(&var1);
spin_lock(&p->lock);
p->count += this_cpu_read(var2);
```

La funzione `migrate_disable()` assicura che il processo venga tenuto sulla CPU
corrente, e di conseguenza garantisce che gli accessi per-CPU alle variabili var1 e
var2 rimangano sulla stessa CPU fintanto che il processo rimane prelabile.

La sostituzione con `migrate_disable()` non funzionerà nel seguente scenario:

```
func()
{
  struct foo *p;

  migrate_disable();
  p = this_cpu_ptr(&var1);
  p->val = func2();
```

Questo non funziona perché `migrate_disable()` non protegge dal ritorno da un
processo che aveva avuto il diritto di prelazione. Una sostituzione più adatta
per questo caso è:

```
func()
{
  struct foo *p;

  local_lock(&foo_lock);
  p = this_cpu_ptr(&var1);
  p->val = func2();
```

Su un kernel non-PREEMPT\_RT, questo codice protegge dal rientro disabilitando la
prelazione. Su un kernel PREEMPT\_RT si ottiene lo stesso risultato acquisendo lo
spinlock di CPU.

### raw\_spinlock\_t su RT

Acquisire un raw\_spinlock\_t disabilita la prelazione e possibilmente anche le
interruzioni, quindi la sezione critica deve evitare di acquisire uno spinlock\_t
o rwlock\_t. Per esempio, la sezione critica non deve fare allocazioni di
memoria. Su un kernel non-PREEMPT\_RT il seguente codice funziona perfettamente:

```
raw_spin_lock(&lock);
p = kmalloc(sizeof(*p), GFP_ATOMIC);
```

Ma lo stesso codice non funziona su un kernel PREEMPT\_RT perché l’allocatore di
memoria può essere oggetto di prelazione e quindi non può essere chiamato in un
contesto atomico. Tuttavia, si può chiamare l’allocatore di memoria quando si
trattiene un blocco *non-raw* perché non disabilitano la prelazione sui kernel
PREEMPT\_RT:

```
spin_lock(&lock);
p = kmalloc(sizeof(*p), GFP_ATOMIC);
```

### bit spinlocks

I kernel PREEMPT\_RT non possono sostituire i bit spinlock perché un singolo bit
è troppo piccolo per farci stare un rtmutex. Dunque, la semantica dei bit
spinlock è mantenuta anche sui kernel PREEMPT\_RT. Quindi, le precisazioni fatte
per raw\_spinlock\_t valgono anche qui.

In PREEMPT\_RT, alcuni bit spinlock sono sostituiti con normali spinlock\_t usando
condizioni di preprocessore in base a dove vengono usati. Per contro, questo non
serve quando si sostituiscono gli spinlock\_t. Invece, le condizioni poste sui
file d’intestazione e sul cuore dell’implementazione della sincronizzazione
permettono al compilatore di effettuare la sostituzione in modo trasparente.

## Regole d’annidamento dei tipi di blocchi

Le regole principali sono:

> * I tipi di blocco appartenenti alla stessa categoria possono essere annidati
>   liberamente a patto che si rispetti l’ordine di blocco al fine di evitare
>   stalli.
> * I blocchi con sospensione non possono essere annidati in blocchi del tipo
>   CPU locale o ad attesa attiva
> * I blocchi ad attesa attiva e su CPU locale possono essere annidati nei
>   blocchi ad attesa con sospensione.
> * I blocchi ad attesa attiva possono essere annidati in qualsiasi altro tipo.

Queste limitazioni si applicano ad entrambe i kernel con o senza PREEMPT\_RT.

Il fatto che un kernel PREEMPT\_RT cambi i blocchi spinlock\_t e rwlock\_t dal tipo
ad attesa attiva a quello con sospensione, e che sostituisca local\_lock con uno
spinlock\_t per CPU, significa che non possono essere acquisiti quando si è in un
blocco raw\_spinlock. Ne consegue il seguente ordine d’annidamento:

> 1. blocchi ad attesa con sospensione
> 2. spinlock\_t, rwlock\_t, local\_lock
> 3. raw\_spinlock\_t e bit spinlocks

Se queste regole verranno violate, allora lockdep se ne accorgerà e questo sia
con o senza PREEMPT\_RT.
