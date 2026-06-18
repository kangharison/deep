# Notas sobre o processo de desenvolvimento de subsistemas e mantenedores

> 출처(원문): https://docs.kernel.org/translations/pt_BR/process/maintainer-handbooks.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Notas sobre o processo de desenvolvimento de subsistemas e mantenedores

O propósito deste documento é fornecer informações específicas de
subsistemas que são suplementares ao manual geral do processo de
desenvolvimento.

Conteúdos:

* [1. Subsistema de Rede do Linux (netdev)](maintainer-netdev.html)
  + [1.1. tl;dr](maintainer-netdev.html#tl-dr)
  + [1.2. netdev](maintainer-netdev.html#netdev)
  + [1.3. Ciclo de Desenvolvimento](maintainer-netdev.html#ciclo-de-desenvolvimento)
  + [1.4. Árvores git e fluxo de patches](maintainer-netdev.html#arvores-git-e-fluxo-de-patches)
  + [1.5. Revisão de patches da netdev](maintainer-netdev.html#revisao-de-patches-da-netdev)
  + [1.6. Atualizando o status do patch](maintainer-netdev.html#atualizando-o-status-do-patch)
  + [1.7. Preparando as mudanças](maintainer-netdev.html#preparando-as-mudancas)
  + [1.8. Limitar patches pendentes na lista de discussão](maintainer-netdev.html#limitar-patches-pendentes-na-lista-de-discussao)
  + [1.9. Testes](maintainer-netdev.html#testes)
  + [1.10. Status de suporte para drivers](maintainer-netdev.html#status-de-suporte-para-drivers)
  + [1.11. Orientações para revisores](maintainer-netdev.html#orientacoes-para-revisores)
  + [1.12. Depoimentos / feedback](maintainer-netdev.html#depoimentos-feedback)
* [2. Subsistema SoC](maintainer-soc.html)
  + [2.1. Visão Geral](maintainer-soc.html#visao-geral)
  + [2.2. Mantenedores](maintainer-soc.html#mantenedores)
  + [2.3. Informações para (novos) Submantenedores](maintainer-soc.html#informacoes-para-novos-submantenedores)
* [3. Plataformas SoC com Requisitos de Conformidade de DTS](maintainer-soc-clean-dts.html)
  + [3.1. Visão Geral](maintainer-soc-clean-dts.html#visao-geral)
  + [3.2. Conformidade Estrita com DT Schema de DTS e dtc](maintainer-soc-clean-dts.html#conformidade-estrita-com-dt-schema-de-dts-e-dtc)
