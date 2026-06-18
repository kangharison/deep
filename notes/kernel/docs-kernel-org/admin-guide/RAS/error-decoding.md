# Error decoding

> 출처(원문): https://docs.kernel.org/admin-guide/RAS/error-decoding.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Error decoding

## x86

Error decoding on AMD systems should be done using the rasdaemon tool:
<https://github.com/mchehab/rasdaemon/>

While the daemon is running, it would automatically log and decode
errors. If not, one can still decode such errors by supplying the
hardware information from the error:

```
$ rasdaemon -p --status <STATUS> --ipid <IPID> --smca
```

Also, the user can pass particular family and model to decode the error
string:

```
$ rasdaemon -p --status <STATUS> --ipid <IPID> --smca --family <CPU Family> --model <CPU Model> --bank <BANK_NUM>
```
