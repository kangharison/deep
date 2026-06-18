# Git配置

> 출처(원문): https://docs.kernel.org/translations/zh_CN/maintainer/configure-git.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Configuring Git](../../../maintainer/configure-git.html)

译者:
:   吴想成 Wu XiangCheng <[bobwxc@email.cn](mailto:bobwxc%40email.cn)>

# Git配置

本章讲述了维护者级别的git配置。

[Creating Pull Requests](../../../maintainer/pull-requests.html) 中使用的标记分支应使用开发人员的
GPG公钥进行签名。可以通过将 `-u` 标志传递给 `git tag` 来创建签名标记。
但是，由于 *通常* 对同一项目使用同一个密钥，因此可以设置:

```
git config user.signingkey "keyname"
```

或者手动编辑你的 `.git/config` 或 `~/.gitconfig` 文件:

```
[user]
        name = Jane Developer
        email = jd@domain.org
        signingkey = jd@domain.org
```

你可能需要告诉 `git` 去使用 `gpg2`:

```
[gpg]
        program = /path/to/gpg2
```

你可能也需要告诉 `gpg` 去使用哪个 `tty` （添加到你的shell rc文件中）:

```
export GPG_TTY=$(tty)
```

## 创建链接到lore.kernel.org的提交

<http://lore.kernel.org> 网站是所有涉及或影响内核开发的邮件列表的总存档。在这里
存储补丁存档是推荐的做法，当维护人员将补丁应用到子系统树时，最好提供一个指向
lore存档链接的标签，以便浏览提交历史的人可以找到某个更改背后的相关讨论和基本
原理。链接标签如下所示：

> Link: <https://lore.kernel.org/r>/<message-id>

通过在git中添加以下钩子，可以将此配置为在发布 `git am` 时自动执行：

```
$ git config am.messageid true
$ cat >.git/hooks/applypatch-msg <<'EOF'
#!/bin/sh
. git-sh-setup
perl -pi -e 's|^Message-Id:\s*<?([^>]+)>?$|Link: https://lore.kernel.org/r/$1|g;' "$1"
test -x "$GIT_DIR/hooks/commit-msg" &&
        exec "$GIT_DIR/hooks/commit-msg" ${1+"$@"}
:
EOF
$ chmod a+x .git/hooks/applypatch-msg
```
