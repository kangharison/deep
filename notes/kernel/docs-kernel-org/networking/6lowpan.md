# Netdev private dataroom for 6lowpan interfaces

> 출처(원문): https://docs.kernel.org/networking/6lowpan.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Netdev private dataroom for 6lowpan interfaces

All 6lowpan able net devices, means all interfaces with ARPHRD\_6LOWPAN,
must have “`struct lowpan_priv`” placed at beginning of netdev\_priv.

The priv\_size of each interface should be calculate by:

```
dev->priv_size = LOWPAN_PRIV_SIZE(LL_6LOWPAN_PRIV_DATA);
```

Where LL\_PRIV\_6LOWPAN\_DATA is sizeof linklayer 6lowpan private data struct.
To access the LL\_PRIV\_6LOWPAN\_DATA structure you can cast:

```
lowpan_priv(dev)-priv;
```

to your LL\_6LOWPAN\_PRIV\_DATA structure.

Before registering the lowpan netdev interface you must run:

```
lowpan_netdev_setup(dev, LOWPAN_LLTYPE_FOOBAR);
```

wheres LOWPAN\_LLTYPE\_FOOBAR is a define for your 6LoWPAN linklayer type of
`enum lowpan_lltypes`.

Example to evaluate the private usually you can do:

```
static inline struct lowpan_priv_foobar *
lowpan_foobar_priv(struct net_device *dev)
{
       return (struct lowpan_priv_foobar *)lowpan_priv(dev)->priv;
}

switch (dev->type) {
case ARPHRD_6LOWPAN:
       lowpan_priv = lowpan_priv(dev);
       /* do great stuff which is ARPHRD_6LOWPAN related */
       switch (lowpan_priv->lltype) {
       case LOWPAN_LLTYPE_FOOBAR:
               /* do 802.15.4 6LoWPAN handling here */
               lowpan_foobar_priv(dev)->bar = foo;
               break;
       ...
       }
       break;
...
}
```

In case of generic 6lowpan branch (“net/6lowpan”) you can remove the check
on ARPHRD\_6LOWPAN, because you can be sure that these function are called
by ARPHRD\_6LOWPAN interfaces.
