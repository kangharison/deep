# Guía del proceso de desarrollo del kernel

> 출처(원문): https://docs.kernel.org/translations/sp_SP/process/development-process.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Warning

Si tiene alguna duda sobre la exactitud del contenido de esta
traducción, la única referencia válida es la documentación oficial en
inglés.
Además, por defecto, los enlaces a documentos redirigen a la
documentación en inglés, incluso si existe una versión traducida.
Consulte el índice para más información.

Original:
:   [A guide to the Kernel Development Process](../../../process/development-process.html)

Translator:
:   Carlos Bilbao <[carlos.bilbao.osdev@gmail.com](mailto:carlos.bilbao.osdev%40gmail.com)> and Avadhut Naik <[avadhut.naik@amd.com](mailto:avadhut.naik%40amd.com)>

# Guía del proceso de desarrollo del kernel

El propósito de este documento es ayudar a los desarrolladores (y sus
jefes) a trabajar con la comunidad de desarrollo con el mínimo de
frustración. Es un intento de documentar cómo funciona esta comunidad
de una manera accesible, para aquellos que no están familiarizados
íntimamente con el desarrollo del kernel Linux (o, de hecho, el desarrollo
de software libre en general). Si bien hay algo de material técnico aquí,
esto es en gran medida una discusión orientada al proceso que no requiere
un conocimiento profundo de la programación del kernel para entenderla.

Contenido

* [1. Introducción](1.Intro.html)
  + [1.1. Resumen ejecutivo](1.Intro.html#resumen-ejecutivo)
  + [1.2. De qué trata este documento](1.Intro.html#de-que-trata-este-documento)
  + [1.3. Créditos](1.Intro.html#creditos)
  + [1.4. Importancia de integrar el código en el mainline](1.Intro.html#importancia-de-integrar-el-codigo-en-el-mainline)
  + [1.5. Licencias](1.Intro.html#licencias)
* [2. Cómo funciona el proceso de desarrollo](2.Process.html)
  + [2.1. El panorama general](2.Process.html#el-panorama-general)
  + [2.2. Ciclo de vida de un parche](2.Process.html#ciclo-de-vida-de-un-parche)
  + [2.3. Cómo se integran los parches en el kernel](2.Process.html#como-se-integran-los-parches-en-el-kernel)
  + [2.4. Árboles siguientes (next)](2.Process.html#arboles-siguientes-next)
  + [2.5. Árboles de staging](2.Process.html#arboles-de-staging)
  + [2.6. Herramientas](2.Process.html#herramientas)
  + [2.7. Listas de correo](2.Process.html#listas-de-correo)
  + [2.8. Comenzar con el desarrollo del kernel](2.Process.html#comenzar-con-el-desarrollo-del-kernel)
* [3. Planificación en etapa inicial](3.Early-stage.html)
  + [3.1. Especificar el problema](3.Early-stage.html#especificar-el-problema)
  + [3.2. Discusión temprana](3.Early-stage.html#discusion-temprana)
  + [3.3. ¿Con quién hablar?](3.Early-stage.html#con-quien-hablar)
  + [3.4. ¿Cuándo publicar?](3.Early-stage.html#cuando-publicar)
  + [3.5. Obtener respaldo oficial](3.Early-stage.html#obtener-respaldo-oficial)
* [4. Conseguir el código correcto](4.Coding.html)
  + [4.1. Problemas](4.Coding.html#problemas)
* [5. Publicación de parches](5.Posting.html)
  + [5.1. Cuando publicar](5.Posting.html#cuando-publicar)
  + [5.2. Antes de crear parches](5.Posting.html#antes-de-crear-parches)
  + [5.3. Preparación del parche](5.Posting.html#preparacion-del-parche)
  + [5.4. Formato de parches y registros de cambios](5.Posting.html#formato-de-parches-y-registros-de-cambios)
  + [5.5. Envió del parche](5.Posting.html#envio-del-parche)
* [6. Seguimiento](6.Followthrough.html)
  + [6.1. Trabajando con revisores](6.Followthrough.html#trabajando-con-revisores)
  + [6.2. ¿Qué pasa después?](6.Followthrough.html#que-pasa-despues)
  + [6.3. Otras cosas que pueden suceder](6.Followthrough.html#otras-cosas-que-pueden-suceder)
* [7. Temas avanzados](7.AdvancedTopics.html)
  + [7.1. Gestionar parches con git](7.AdvancedTopics.html#gestionar-parches-con-git)
  + [7.2. Revisión de parches](7.AdvancedTopics.html#revision-de-parches)
* [8. Para más información](8.Conclusion.html)
* [9. Conclusión](8.Conclusion.html#conclusion)
