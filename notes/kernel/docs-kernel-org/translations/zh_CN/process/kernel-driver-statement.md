# 内核驱动声明

> 출처(원문): https://docs.kernel.org/translations/zh_CN/process/kernel-driver-statement.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Documentation/process/kernel-driver-statement.rst](../../../process/kernel-driver-statement.html#process-statement-driver)

Translator:
:   Alex Shi <[alex.shi@linux.alibaba.com](mailto:alex.shi%40linux.alibaba.com)>

# 内核驱动声明

## 关于Linux内核模块的立场声明

我们，以下署名的Linux内核开发人员，认为任何封闭源Linux内核模块或驱动程序都是
有害的和不可取的。我们已经一再发现它们对Linux用户，企业和更大的Linux生态系统
有害。这样的模块否定了Linux开发模型的开放性，稳定性，灵活性和可维护性，并使
他们的用户无法使用Linux社区的专业知识。提供闭源内核模块的供应商迫使其客户
放弃Linux的主要优势或选择新的供应商。因此，为了充分利用开源所提供的成本节省和
共享支持优势，我们敦促供应商采取措施，以开源内核代码在Linux上为其客户提供支持。

我们只为自己说话，而不是我们今天可能会为之工作，过去或将来会为之工作的任何公司。

> * Dave Airlie
> * Nick Andrew
> * Jens Axboe
> * Ralf Baechle
> * Felipe Balbi
> * Ohad Ben-Cohen
> * Muli Ben-Yehuda
> * Jiri Benc
> * Arnd Bergmann
> * Thomas Bogendoerfer
> * Vitaly Bordug
> * James Bottomley
> * Josh Boyer
> * Neil Brown
> * Mark Brown
> * David Brownell
> * Michael Buesch
> * Franck Bui-Huu
> * Adrian Bunk
> * François Cami
> * Ralph Campbell
> * Luiz Fernando N. Capitulino
> * Mauro Carvalho Chehab
> * Denis Cheng
> * Jonathan Corbet
> * Glauber Costa
> * Alan Cox
> * Magnus Damm
> * Ahmed S. Darwish
> * Robert P. J. Day
> * Hans de Goede
> * Arnaldo Carvalho de Melo
> * Helge Deller
> * Jean Delvare
> * Mathieu Desnoyers
> * Sven-Thorsten Dietrich
> * Alexey Dobriyan
> * Daniel Drake
> * Alex Dubov
> * Randy Dunlap
> * Michael Ellerman
> * Pekka Enberg
> * Jan Engelhardt
> * Mark Fasheh
> * 10. Bruce Fields
> * Larry Finger
> * Jeremy Fitzhardinge
> * Mike Frysinger
> * Kumar Gala
> * Robin Getz
> * Liam Girdwood
> * Jan-Benedict Glaw
> * Thomas Gleixner
> * Brice Goglin
> * Cyrill Gorcunov
> * Andy Gospodarek
> * Thomas Graf
> * Krzysztof Halasa
> * Harvey Harrison
> * Stephen Hemminger
> * Michael Hennerich
> * Tejun Heo
> * Benjamin Herrenschmidt
> * Kristian Høgsberg
> * Henrique de Moraes Holschuh
> * Marcel Holtmann
> * Mike Isely
> * Takashi Iwai
> * Olof Johansson
> * Dave Jones
> * Jesper Juhl
> * Matthias Kaehlcke
> * Kenji Kaneshige
> * Jan Kara
> * Jeremy Kerr
> * Russell King
> * Olaf Kirch
> * Roel Kluin
> * Hans-Jürgen Koch
> * Auke Kok
> * Peter Korsgaard
> * Jiri Kosina
> * Aaro Koskinen
> * Mariusz Kozlowski
> * Greg Kroah-Hartman
> * Michael Krufky
> * Aneesh Kumar
> * Clemens Ladisch
> * Christoph Lameter
> * Gunnar Larisch
> * Anders Larsen
> * Grant Likely
> * John W. Linville
> * Yinghai Lu
> * Tony Luck
> * Pavel Machek
> * Matt Mackall
> * Paul Mackerras
> * Roland McGrath
> * Patrick McHardy
> * Kyle McMartin
> * Paul Menage
> * Thierry Merle
> * Eric Miao
> * Akinobu Mita
> * Ingo Molnar
> * James Morris
> * Andrew Morton
> * Paul Mundt
> * Oleg Nesterov
> * Luca Olivetti
> * S.Çağlar Onur
> * Pierre Ossman
> * Keith Owens
> * Venkatesh Pallipadi
> * Nick Piggin
> * Nicolas Pitre
> * Evgeniy Polyakov
> * Richard Purdie
> * Mike Rapoport
> * Sam Ravnborg
> * Gerrit Renker
> * Stefan Richter
> * David Rientjes
> * Luis R. Rodriguez
> * Stefan Roese
> * Francois Romieu
> * Rami Rosen
> * Stephen Rothwell
> * Maciej W. Rozycki
> * Mark Salyzyn
> * Yoshinori Sato
> * Deepak Saxena
> * Holger Schurig
> * Amit Shah
> * Yoshihiro Shimoda
> * Sergei Shtylyov
> * Kay Sievers
> * Sebastian Siewior
> * Rik Snel
> * Jes Sorensen
> * Alexey Starikovskiy
> * Alan Stern
> * Timur Tabi
> * Hirokazu Takata
> * Eliezer Tamir
> * Eugene Teo
> * Doug Thompson
> * FUJITA Tomonori
> * Dmitry Torokhov
> * Marcelo Tosatti
> * Steven Toth
> * Theodore Tso
> * Matthias Urlichs
> * Geert Uytterhoeven
> * Arjan van de Ven
> * Ivo van Doorn
> * Rik van Riel
> * Wim Van Sebroeck
> * Hans Verkuil
> * Horst H. von Brand
> * Dmitri Vorobiev
> * Anton Vorontsov
> * Daniel Walker
> * Johannes Weiner
> * Harald Welte
> * Matthew Wilcox
> * Dan J. Williams
> * Darrick J. Wong
> * David Woodhouse
> * Chris Wright
> * Bryan Wu
> * Rafael J. Wysocki
> * Herbert Xu
> * Vlad Yasevich
> * Peter Zijlstra
> * Bartlomiej Zolnierkiewicz
