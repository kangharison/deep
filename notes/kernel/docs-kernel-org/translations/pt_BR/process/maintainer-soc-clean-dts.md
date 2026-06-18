# 3.Plataformas SoC com Requisitos de Conformidade de DTS

> 출처(원문): https://docs.kernel.org/translations/pt_BR/process/maintainer-soc-clean-dts.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3. Plataformas SoC com Requisitos de Conformidade de DTS

## 3.1. Visão Geral

As plataformas SoC ou subarquiteturas devem seguir todas as regras de
[SoC Subsystem](../../../process/maintainer-soc.html). Este documento, referenciado em
MAINTAINERS, impõe requisitos adicionais listados abaixo.

## 3.2. Conformidade Estrita com DT Schema de DTS e dtc

Nenhuma alteração nos arquivos de origem do Devicetree da plataforma SoC
(arquivos DTS) deve introduzir novos avisos de `make dtbs_check W=1`.
Avisos em um novo DTS de placa, que sejam resultado de problemas em um
arquivo DTSI incluído, são considerados avisos existentes, não novos.
Para séries divididas entre árvores diferentes (vínculos de DT seguem pela
árvore do subsistema de drivers), os avisos no linux-next são decisivos.
Os mantenedores da plataforma possuem automação implementada que deve
apontar quaisquer novos avisos.

Se um commit que introduz novos avisos for aceito de alguma forma, os
problemas resultantes deverão ser corrigidos em um tempo razoável
(por exemplo, dentro de um ciclo de lançamento) ou o commit será revertido.
