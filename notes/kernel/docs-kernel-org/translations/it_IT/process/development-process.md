# Una guida al processo di sviluppo del Kernel

> 출처(원문): https://docs.kernel.org/translations/it_IT/process/development-process.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Warning

In caso di dubbi sulla correttezza del contenuto di questa traduzione,
l’unico riferimento valido è la documentazione ufficiale in inglese.
Per maggiori informazioni consultate le [avvertenze](../index.html#it-disclaimer).

Original:
:   [Documentation/process/development-process.rst](../../../process/development-process.html#development-process-main)

Translator:
:   Alessia Mantegazza <[amantegazza@vaga.pv.it](mailto:amantegazza%40vaga.pv.it)>

# Una guida al processo di sviluppo del Kernel

Lo scopo di questo documento è quello di aiutare gli sviluppatori (ed i loro
supervisori) a lavorare con la communità di sviluppo con il minimo sforzo. È
un tentativo di documentare il funzionamento di questa communità in modo che
sia accessibile anche a coloro che non hanno famigliarità con lo sviluppo del
Kernel Linux (o, anzi, con lo sviluppo di software libero in generale). Benchè
qui sia presente del materiale tecnico, questa è una discussione rivolta in
particolare al procedimento, e quindi per essere compreso non richiede una
conoscenza approfondità sullo sviluppo del kernel.

Contenuti

* [1. Introduzione](1.Intro.html)
  + [1.1. Riepilogo generale](1.Intro.html#riepilogo-generale)
  + [1.2. Di cosa parla questo documento](1.Intro.html#di-cosa-parla-questo-documento)
  + [1.3. Crediti](1.Intro.html#crediti)
  + [1.4. L’importanza d’avere il codice nei sorgenti principali](1.Intro.html#l-importanza-d-avere-il-codice-nei-sorgenti-principali)
  + [1.5. Licenza](1.Intro.html#licenza)
* [2. Come funziona il processo di sviluppo](2.Process.html)
  + [2.1. Il quadro d’insieme](2.Process.html#il-quadro-d-insieme)
  + [2.2. Il ciclo di vita di una patch](2.Process.html#il-ciclo-di-vita-di-una-patch)
  + [2.3. Come le modifiche finiscono nel Kernel](2.Process.html#come-le-modifiche-finiscono-nel-kernel)
  + [2.4. Sorgenti -next](2.Process.html#sorgenti-next)
  + [2.5. Sorgenti in preparazione](2.Process.html#sorgenti-in-preparazione)
  + [2.6. Strumenti](2.Process.html#strumenti)
  + [2.7. Liste di discussione](2.Process.html#liste-di-discussione)
  + [2.8. Iniziare con lo sviluppo del Kernel](2.Process.html#iniziare-con-lo-sviluppo-del-kernel)
* [3. I primi passi della pianificazione](3.Early-stage.html)
  + [3.1. Specificare il problema](3.Early-stage.html#specificare-il-problema)
  + [3.2. Prime discussioni](3.Early-stage.html#prime-discussioni)
  + [3.3. Con chi parlare?](3.Early-stage.html#con-chi-parlare)
  + [3.4. Quando pubblicare](3.Early-stage.html#quando-pubblicare)
  + [3.5. Ottenere riscontri ufficiali](3.Early-stage.html#ottenere-riscontri-ufficiali)
* [4. Scrivere codice corretto](4.Coding.html)
  + [4.1. Trappole](4.Coding.html#trappole)
  + [4.2. Strumenti di verifica del codice](4.Coding.html#strumenti-di-verifica-del-codice)
  + [4.3. Documentazione](4.Coding.html#documentazione)
  + [4.4. Cambiamenti interni dell’API](4.Coding.html#cambiamenti-interni-dell-api)
* [5. Pubblicare modifiche](5.Posting.html)
  + [5.1. Quando pubblicarle](5.Posting.html#quando-pubblicarle)
  + [5.2. Prima di creare patch](5.Posting.html#prima-di-creare-patch)
  + [5.3. Preparazione di una patch](5.Posting.html#preparazione-di-una-patch)
  + [5.4. Formattazione delle patch e i changelog](5.Posting.html#formattazione-delle-patch-e-i-changelog)
  + [5.5. Inviare la modifica](5.Posting.html#inviare-la-modifica)
* [6. Completamento](6.Followthrough.html)
  + [6.1. Lavorare con i revisori](6.Followthrough.html#lavorare-con-i-revisori)
  + [6.2. Cosa accade poi](6.Followthrough.html#cosa-accade-poi)
  + [6.3. Altre cose che posso accadere](6.Followthrough.html#altre-cose-che-posso-accadere)
* [7. Argomenti avanzati](7.AdvancedTopics.html)
  + [7.1. Gestire le modifiche con git](7.AdvancedTopics.html#gestire-le-modifiche-con-git)
  + [7.2. Revisionare le patch](7.AdvancedTopics.html#revisionare-le-patch)
* [8. Per maggiori informazioni](8.Conclusion.html)
* [9. Conclusioni](8.Conclusion.html#conclusioni)
