# 创建虚拟串口

```
sudo socat -d -d pty,raw,echo=0 pty,raw,echo=0

sudo cat /dev/pts/0
sudo sh -c "echo test > /dev/pts/3"
```
