# Includere gli i file di intestazione uAPI

> 출처(원문): https://docs.kernel.org/translations/it_IT/doc-guide/parse-headers.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Warning

In caso di dubbi sulla correttezza del contenuto di questa traduzione,
l’unico riferimento valido è la documentazione ufficiale in inglese.
Per maggiori informazioni consultate le [avvertenze](../index.html#it-disclaimer).

Original:
:   [How to write kernel documentation](../../../doc-guide/index.html)

# Includere gli i file di intestazione uAPI

Qualche volta è utile includere dei file di intestazione e degli esempi di codice C
al fine di descrivere l’API per lo spazio utente e per generare dei riferimenti
fra il codice e la documentazione. Aggiungere i riferimenti ai file dell’API
dello spazio utente ha ulteriori vantaggi: Sphinx genererà dei messaggi
d’avviso se un simbolo non viene trovato nella documentazione. Questo permette
di mantenere allineate la documentazione della uAPI (API spazio utente)
con le modifiche del kernel.
Il programma [parse\_headers.py](#it-parse-headers) genera questi riferimenti.
Esso dev’essere invocato attraverso un Makefile, mentre si genera la
documentazione. Per avere un esempio su come utilizzarlo all’interno del kernel
consultate `Documentation/userspace-api/media/Makefile`.

## parse\_headers.py

### NOME

parse\_headers.py - analizza i file C al fine di identificare funzioni,
strutture, enumerati e definizioni, e creare riferimenti per Sphinx

### SINTASSI

**parse\_headers.py** [<options>] <C\_FILE> <OUT\_FILE> [<EXCEPTIONS\_FILE>]

Dove <options> può essere: --debug, --usage o --help.

### OPZIONI

**--debug**

> Lo script viene messo in modalità verbosa, utile per il debugging.

**--usage**

> Mostra un messaggio d’aiuto breve e termina.

**--help**

> Mostra un messaggio d’aiuto dettagliato e termina.

### DESCRIZIONE

Converte un file d’intestazione o un file sorgente C (C\_FILE) in un testo
reStructuredText incluso mediante il blocco ..parsed-literal
con riferimenti alla documentazione che descrive l’API. Opzionalmente,
il programma accetta anche un altro file (EXCEPTIONS\_FILE) che
descrive quali elementi debbano essere ignorati o il cui riferimento
deve puntare ad elemento diverso dal predefinito.

Il file generato sarà disponibile in (OUT\_FILE).

Il programma è capace di identificare *define*, funzioni, strutture,
tipi di dato, enumerati e valori di enumerati, e di creare i riferimenti
per ognuno di loro. Inoltre, esso è capace di distinguere le #define
utilizzate per specificare i comandi ioctl di Linux.

Il file EXCEPTIONS\_FILE contiene due tipi di dichiarazioni:
**ignore** o **replace**.

La sintassi per ignore è:

ignore **tipo** **nome**

La dichiarazione **ignore** significa che non verrà generato alcun
riferimento per il simbolo **name** di tipo **tipo**.

La sintassi per replace è:

replace **tipo** **nome** **nuovo\_valore**

La dichiarazione **replace** significa che verrà generato un
riferimento per il simbolo **name**di tipo **tipo**, ma, invece
di utilizzare il valore predefinito, verrà utilizzato il valore
**nuovo\_valore**.

Per entrambe le dichiarazioni, il **tipo** può essere uno dei seguenti:

**ioctl**

> La dichiarazione ignore o replace verrà applicata su definizioni di ioctl
> come la seguente:
>
> #define VIDIOC\_DBG\_S\_REGISTER \_IOW(‘V’, 79, `struct v4l2_dbg_register`)

**define**

> La dichiarazione ignore o replace verrà applicata su una qualsiasi #define
> trovata in C\_FILE.

**typedef**

> La dichiarazione ignore o replace verrà applicata ad una dichiarazione `typedef
> in` C\_FILE.

**struct**

> La dichiarazione ignore o replace verrà applicata ai nomi di strutture
> in C\_FILE.

**enum**

> La dichiarazione ignore o replace verrà applicata ai nomi di enumerati
> in C\_FILE.

**symbol**

> La dichiarazione ignore o replace verrà applicata ai nomi di valori di
> enumerati in C\_FILE.
>
> Per le dichiarazioni di tipo replace, il campo **new\_value** utilizzerà
> automaticamente i riferimenti :c:type: per **typedef**, **enum** e
> **struct**. Invece, utilizzerà :ref: per **ioctl**, **define** e
> **symbol**. Il tipo di riferimento può essere definito esplicitamente
> nella dichiarazione stessa.

### ESEMPI

ignore define \_VIDEODEV2\_H

Ignora una definizione #define \_VIDEODEV2\_H nel file C\_FILE.

ignore symbol PRIVATE

In un enumerato come il seguente:

`enum foo` { BAR1, BAR2, PRIVATE };

Non genererà alcun riferimento per **PRIVATE**.

replace symbol BAR1 :c:type:`foo`
replace symbol BAR2 :c:type:`foo`

In un enumerato come il seguente:

`enum foo` { BAR1, BAR2, PRIVATE };

Genererà un riferimento ai valori BAR1 e BAR2 dal simbolo foo nel dominio C.

### BUGS

Riferire ogni malfunzionamento a Mauro Carvalho Chehab <[mchehab@s-opensource.com](mailto:mchehab%40s-opensource.com)>

### COPYRIGHT

Copyright (c) 2016 by Mauro Carvalho Chehab <[mchehab@s-opensource.com](mailto:mchehab%40s-opensource.com)>.

Licenza GPLv2: GNU GPL version 2 <<https://gnu.org/licenses/gpl.html>>.

Questo è software libero: siete liberi di cambiarlo e ridistribuirlo.
Non c’è alcuna garanzia, nei limiti permessi dalla legge.
