# netns_ipv4 enum fast path usage breakdown

> 출처(원문): https://docs.kernel.org/networking/net_cachelines/snmp.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# netns\_ipv4 enum fast path usage breakdown

| Type | Name | fastpath\_tx\_access | fastpath\_rx\_access | comment |
| --- | --- | --- | --- | --- |
| unsigned\_long | LINUX\_MIB\_TCPKEEPALIVE | write\_mostly |  | tcp\_keepalive\_timer |
| unsigned\_long | LINUX\_MIB\_DELAYEDACKS | write\_mostly |  | tcp\_delack\_timer\_handler,tcp\_delack\_timer |
| unsigned\_long | LINUX\_MIB\_DELAYEDACKLOCKED | write\_mostly |  | tcp\_delack\_timer\_handler,tcp\_delack\_timer |
| unsigned\_long | LINUX\_MIB\_TCPAUTOCORKING | write\_mostly |  | tcp\_push,tcp\_sendmsg\_locked |
| unsigned\_long | LINUX\_MIB\_TCPFROMZEROWINDOWADV | write\_mostly |  | tcp\_select\_window,tcp\_transmit-skb |
| unsigned\_long | LINUX\_MIB\_TCPTOZEROWINDOWADV | write\_mostly |  | tcp\_select\_window,tcp\_transmit-skb |
| unsigned\_long | LINUX\_MIB\_TCPWANTZEROWINDOWADV | write\_mostly |  | tcp\_select\_window,tcp\_transmit-skb |
| unsigned\_long | LINUX\_MIB\_TCPORIGDATASENT | write\_mostly |  | tcp\_write\_xmit |
| unsigned\_long | LINUX\_MIB\_TCPHPHITS |  | write\_mostly | tcp\_rcv\_established,tcp\_v4\_do\_rcv,tcp\_v6\_do\_rcv |
| unsigned\_long | LINUX\_MIB\_TCPRCVCOALESCE |  | write\_mostly | tcp\_try\_coalesce,tcp\_queue\_rcv,tcp\_rcv\_established |
| unsigned\_long | LINUX\_MIB\_TCPPUREACKS |  | write\_mostly | tcp\_ack,tcp\_rcv\_established |
| unsigned\_long | LINUX\_MIB\_TCPHPACKS |  | write\_mostly | tcp\_ack,tcp\_rcv\_established |
| unsigned\_long | LINUX\_MIB\_TCPDELIVERED |  | write\_mostly | tcp\_newly\_delivered,tcp\_ack,tcp\_rcv\_established |
| unsigned\_long | LINUX\_MIB\_SYNCOOKIESSENT |  |  |  |
| unsigned\_long | LINUX\_MIB\_SYNCOOKIESRECV |  |  |  |
| unsigned\_long | LINUX\_MIB\_SYNCOOKIESFAILED |  |  |  |
| unsigned\_long | LINUX\_MIB\_EMBRYONICRSTS |  |  |  |
| unsigned\_long | LINUX\_MIB\_PRUNECALLED |  |  |  |
| unsigned\_long | LINUX\_MIB\_RCVPRUNED |  |  |  |
| unsigned\_long | LINUX\_MIB\_OFOPRUNED |  |  |  |
| unsigned\_long | LINUX\_MIB\_OUTOFWINDOWICMPS |  |  |  |
| unsigned\_long | LINUX\_MIB\_LOCKDROPPEDICMPS |  |  |  |
| unsigned\_long | LINUX\_MIB\_ARPFILTER |  |  |  |
| unsigned\_long | LINUX\_MIB\_TIMEWAITED |  |  |  |
| unsigned\_long | LINUX\_MIB\_TIMEWAITRECYCLED |  |  |  |
| unsigned\_long | LINUX\_MIB\_TIMEWAITKILLED |  |  |  |
| unsigned\_long | LINUX\_MIB\_PAWSACTIVEREJECTED |  |  |  |
| unsigned\_long | LINUX\_MIB\_PAWSESTABREJECTED |  |  |  |
| unsigned\_long | LINUX\_MIB\_BEYOND\_WINDOW |  |  |  |
| unsigned\_long | LINUX\_MIB\_TSECR\_REJECTED |  |  |  |
| unsigned\_long | LINUX\_MIB\_PAWS\_OLD\_ACK |  |  |  |
| unsigned\_long | LINUX\_MIB\_PAWS\_TW\_REJECTED |  |  |  |
| unsigned\_long | LINUX\_MIB\_DELAYEDACKLOST |  |  |  |
| unsigned\_long | LINUX\_MIB\_LISTENOVERFLOWS |  |  |  |
| unsigned\_long | LINUX\_MIB\_LISTENDROPS |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPRENORECOVERY |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPSACKRECOVERY |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPSACKRENEGING |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPSACKREORDER |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPRENOREORDER |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPTSREORDER |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPFULLUNDO |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPPARTIALUNDO |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPDSACKUNDO |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPLOSSUNDO |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPLOSTRETRANSMIT |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPRENOFAILURES |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPSACKFAILURES |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPLOSSFAILURES |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPFASTRETRANS |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPSLOWSTARTRETRANS |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPTIMEOUTS |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPLOSSPROBES |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPLOSSPROBERECOVERY |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPRENORECOVERYFAIL |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPSACKRECOVERYFAIL |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPRCVCOLLAPSED |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPDSACKOLDSENT |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPDSACKOFOSENT |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPDSACKRECV |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPDSACKOFORECV |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPABORTONDATA |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPABORTONCLOSE |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPABORTONMEMORY |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPABORTONTIMEOUT |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPABORTONLINGER |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPABORTFAILED |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPMEMORYPRESSURES |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPMEMORYPRESSURESCHRONO |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPSACKDISCARD |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPDSACKIGNOREDOLD |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPDSACKIGNOREDNOUNDO |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPSPURIOUSRTOS |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPMD5NOTFOUND |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPMD5UNEXPECTED |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPMD5FAILURE |  |  |  |
| unsigned\_long | LINUX\_MIB\_SACKSHIFTED |  |  |  |
| unsigned\_long | LINUX\_MIB\_SACKMERGED |  |  |  |
| unsigned\_long | LINUX\_MIB\_SACKSHIFTFALLBACK |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPBACKLOGDROP |  |  |  |
| unsigned\_long | LINUX\_MIB\_PFMEMALLOCDROP |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPMINTTLDROP |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPDEFERACCEPTDROP |  |  |  |
| unsigned\_long | LINUX\_MIB\_IPRPFILTER |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPTIMEWAITOVERFLOW |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPREQQFULLDOCOOKIES |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPREQQFULLDROP |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPRETRANSFAIL |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPBACKLOGCOALESCE |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPOFOQUEUE |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPOFODROP |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPOFOMERGE |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPCHALLENGEACK |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPSYNCHALLENGE |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPFASTOPENACTIVE |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPFASTOPENACTIVEFAIL |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPFASTOPENPASSIVE |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPFASTOPENPASSIVEFAIL |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPFASTOPENLISTENOVERFLOW |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPFASTOPENCOOKIEREQD |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPFASTOPENBLACKHOLE |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPSPURIOUS\_RTX\_HOSTQUEUES |  |  |  |
| unsigned\_long | LINUX\_MIB\_BUSYPOLLRXPACKETS |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPSYNRETRANS |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPHYSTARTTRAINDETECT |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPHYSTARTTRAINCWND |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPHYSTARTDELAYDETECT |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPHYSTARTDELAYCWND |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPACKSKIPPEDSYNRECV |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPACKSKIPPEDPAWS |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPACKSKIPPEDSEQ |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPACKSKIPPEDFINWAIT2 |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPACKSKIPPEDTIMEWAIT |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPACKSKIPPEDCHALLENGE |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPWINPROBE |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPMTUPFAIL |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPMTUPSUCCESS |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPDELIVEREDCE |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPACKCOMPRESSED |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPZEROWINDOWDROP |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPRCVQDROP |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPWQUEUETOOBIG |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPFASTOPENPASSIVEALTKEY |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPTIMEOUTREHASH |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPDUPLICATEDATAREHASH |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPDSACKRECVSEGS |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPDSACKIGNOREDDUBIOUS |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPMIGRATEREQSUCCESS |  |  |  |
| unsigned\_long | LINUX\_MIB\_TCPMIGRATEREQFAILURE |  |  |  |
| unsigned\_long | \_\_LINUX\_MIB\_MAX |  |  |  |
