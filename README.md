# hcisplite
splite large hci log


在分析hcilog的时候，经常会遇见过大的hcilog，因此需要对hci进行分割。



./splithci -h 可以看到帮助信息

splithci -d[dst file dir] -s[src file] -l[MAX file length]

-d指定了目标的存放文件夹
-s指定了源hci文件
-l制定了分割的文件的大小，单位是1KB

最终的文件大小会尽可能的贴近1KB****l

