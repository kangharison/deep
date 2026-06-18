# Guida all’hacking del kernel

> 출처(원문): https://docs.kernel.org/translations/it_IT/kernel-hacking/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Warning

In caso di dubbi sulla correttezza del contenuto di questa traduzione,
l’unico riferimento valido è la documentazione ufficiale in inglese.
Per maggiori informazioni consultate le [avvertenze](../index.html#it-disclaimer).

Original:
:   [Documentation/kernel-hacking/index.rst](../../../kernel-hacking/index.html#kernel-hacking)

Translator:
:   Federico Vaga <[federico.vaga@vaga.pv.it](mailto:federico.vaga%40vaga.pv.it)>

# Guida all’hacking del kernel

* [L’inaffidabile guida all’hacking del kernel Linux](hacking.html)
  + [Introduzione](hacking.html#introduzione)
  + [Gli attori](hacking.html#gli-attori)
  + [Alcune regole basilari](hacking.html#alcune-regole-basilari)
  + [ioctl: non scrivere nuove chiamate di sistema](hacking.html#ioctl-non-scrivere-nuove-chiamate-di-sistema)
  + [La ricetta per uno stallo](hacking.html#la-ricetta-per-uno-stallo)
  + [Alcune delle procedure più comuni](hacking.html#alcune-delle-procedure-piu-comuni)
  + [Code d’attesa `include/linux/wait.h`](hacking.html#code-d-attesa-include-linux-wait-h)
  + [Operazioni atomiche](hacking.html#operazioni-atomiche)
  + [Simboli](hacking.html#simboli)
  + [Procedure e convenzioni](hacking.html#procedure-e-convenzioni)
  + [Mettere le vostre cose nel kernel](hacking.html#mettere-le-vostre-cose-nel-kernel)
  + [Trucchetti del kernel](hacking.html#trucchetti-del-kernel)
  + [Ringraziamenti](hacking.html#ringraziamenti)
* [L’inaffidabile guida alla sincronizzazione](locking.html)
  + [Introduzione](locking.html#introduzione)
  + [Il problema con la concorrenza](locking.html#il-problema-con-la-concorrenza)
  + [Sincronizzazione nel kernel Linux](locking.html#sincronizzazione-nel-kernel-linux)
  + [Contesto di interruzione hardware](locking.html#contesto-di-interruzione-hardware)
  + [Bigino della sincronizzazione](locking.html#bigino-della-sincronizzazione)
  + [Le funzioni *trylock*](locking.html#le-funzioni-trylock)
  + [Esempi più comuni](locking.html#esempi-piu-comuni)
  + [Problemi comuni](locking.html#problemi-comuni)
  + [Velocità della sincronizzazione](locking.html#velocita-della-sincronizzazione)
  + [Quali funzioni possono essere chiamate in modo sicuro dalle interruzioni?](locking.html#quali-funzioni-possono-essere-chiamate-in-modo-sicuro-dalle-interruzioni)
  + [Riferimento per l’API dei Mutex](locking.html#riferimento-per-l-api-dei-mutex)
  + [Riferimento per l’API dei Futex](locking.html#riferimento-per-l-api-dei-futex)
  + [Approfondimenti](locking.html#approfondimenti)
  + [Ringraziamenti](locking.html#ringraziamenti)
  + [Glossario](locking.html#glossario)
