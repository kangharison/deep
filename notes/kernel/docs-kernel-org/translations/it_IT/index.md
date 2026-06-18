# La documentazione del kernel Linux

> 출처(원문): https://docs.kernel.org/translations/it_IT/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# La documentazione del kernel Linux

manutentore:
:   Federico Vaga <[federico.vaga@vaga.pv.it](mailto:federico.vaga%40vaga.pv.it)>

Questo è il livello principale della documentazione del kernel in
lingua italiana. La traduzione è incompleta, noterete degli avvisi
che vi segnaleranno la mancanza di una traduzione o di un gruppo di
traduzioni.

Più in generale, la documentazione, come il kernel stesso, sono in
costante sviluppo; particolarmente vero in quanto stiamo lavorando
alla riorganizzazione della documentazione in modo più coerente.
I miglioramenti alla documentazione sono sempre i benvenuti; per cui,
se vuoi aiutare, iscriviti alla lista di discussione linux-doc presso
vger.kernel.org.

## Avvertenze

L’obiettivo di questa traduzione è di rendere più facile la lettura e
la comprensione per chi non capisce l’inglese o ha dubbi sulla sua
interpretazione, oppure semplicemente per chi preferisce leggere in lingua
italiana. Tuttavia, tenete ben presente che l’*unica* documentazione
ufficiale è quella in lingua inglese: [The Linux Kernel documentation](../../index.html#linux-doc)

La propagazione simultanea a tutte le traduzioni di una modifica in
[The Linux Kernel documentation](../../index.html#linux-doc) è altamente improbabile. I manutentori delle traduzioni -
e i contributori - seguono l’evolversi della documentazione ufficiale e
cercano di mantenere le rispettive traduzioni allineate nel limite del
possibile. Per questo motivo non c’è garanzia che una traduzione sia
aggiornata all’ultima modifica. Se quello che leggete in una traduzione
non corrisponde a quello che leggete nel codice, informate il manutentore
della traduzione e - se potete - verificate anche la documentazione in
inglese.

Una traduzione non è un *fork* della documentazione ufficiale, perciò gli
utenti non vi troveranno alcuna informazione diversa rispetto alla versione
ufficiale. Ogni aggiunta, rimozione o modifica dei contenuti deve essere
fatta prima nei documenti in inglese. In seguito, e quando è possibile, la
stessa modifica dovrebbe essere applicata anche alle traduzioni. I manutentori
delle traduzioni accettano contributi che interessano prettamente l’attività
di traduzione (per esempio, nuove traduzioni, aggiornamenti, correzioni).

Le traduzioni cercano di essere il più possibile accurate ma non è possibile
mappare direttamente una lingua in un’altra. Ogni lingua ha la sua grammatica
e una sua cultura alle spalle, quindi la traduzione di una frase in inglese
potrebbe essere modificata per adattarla all’italiano. Per questo motivo,
quando leggete questa traduzione, potreste trovare alcune differenze di forma
ma che trasmettono comunque il messaggio originale. Nonostante la grande
diffusione di inglesismi nella lingua parlata, quando possibile, questi
verranno sostituiti dalle corrispettive parole italiane

Se avete bisogno d’aiuto per comunicare con la comunità Linux ma non vi sentite
a vostro agio nello scrivere in inglese, potete chiedere aiuto al manutentore
della traduzione.

## Lavorare con la comunità di sviluppo

Le guide fondamentali per l’interazione con la comunità di sviluppo del kernel e
su come vedere il proprio lavoro integrato.

* [Una guida al processo di sviluppo del Kernel](process/development-process.html)
* [Inviare patch: la guida essenziale per vedere il vostro codice nel kernel](process/submitting-patches.html)
* [Code of conduct](process/code-of-conduct.html)
* [All development-process docs](process/index.html)

## Manuali sull’API interna

Di seguito una serie di manuali per gli sviluppatori che hanno bisogno di
interfacciarsi con il resto del kernel.

* [Documentazione dell’API di base](core-api/index.html)
* [Sincronizzazione nel kernel](locking/index.html)
* [Documentazione dei sottosistemi del kernel](subsystem-apis.html)

## Strumenti e processi per lo sviluppo

Di seguito una serie di manuali contenenti informazioni utili a tutti gli
sviluppatori del kernel.

* [Regole sulle licenze](process/license-rules.html)
* [Scrivere la documentazione](doc-guide/index.html)
* [Strumenti di sviluppo](dev-tools/index.html)
* [La guida all'\*hacking\*](kernel-hacking/index.html)

## Documentazione per gli utenti

Di seguito una serie di manuali per gli *utenti* del kernel - ovvero coloro che
stanno cercando di farlo funzionare al meglio per un dato sistema, ma anche
coloro che stanno sviluppando applicazioni che sfruttano l’API verso lo
spazio-utente.

Consultate anche [Linux man pages](https://www.kernel.org/doc/man-pages/), che
vengono mantenuti separatamente dalla documentazione del kernel Linux

## Documentazione relativa ai firmware

Di seguito informazioni sulle aspettative del kernel circa i firmware.

## Documentazione specifica per architettura

## Documentazione varia

Ci sono documenti che sono difficili da inserire nell’attuale organizzazione
della documentazione; altri hanno bisogno di essere migliorati e/o convertiti
nel formato *reStructuredText*; altri sono semplicamente troppo vecchi.
