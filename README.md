# Crypto-Dashboard
A tiny OLED display used to display real-time information on my Cryptocurrency assets. Data obtained from Luno and Binance API

![20210602_004443](https://user-images.githubusercontent.com/80947240/120367656-b7466080-c343-11eb-8219-de9d89befa19.jpg)


This project is powered by an NodeMCU ESP8266 12-E Dev Board, it uses a TZT Real OLED Display measuring 3.12" with a dimension of 256x64 for its display. The display is controlled by the amazing [U8G2 Monochrome display library from Olikraus](https://github.com/olikraus/u8g2/). The time is kept in sync with an `NTPClient`. Below the time display is a scrolling window displaying the current prices of each asset held in my wallets, and in brackets, their percentage change over the last 8 hours (or as defined in the `price_change_delay` variable). Below that are the net wallet value of both exchanges along with their percentage change over the last x hours. NAV, or net asset value, displays the combined wallet values, with percentage of profit in brackets.

How does it work?
1. Upon startup, it connects to the local WiFi Network.
2. It then reads asset balances from Luno and Binance
3. The prices of said assets are then requested, along with their prices 8 hours ago (or as defined in the `price_change_delay` variable)
4. Prices are updated every 5 minutes.

Limitations
1. To calculate the profit percentage in the NAV line, the initial investment ammout is hardcoded. I plan to add the ability to update this value over WiFi soon.
2. Poor memory usage; Initialization takes on average 45s to 50s, during this process execution has a small change of failing as the ESP8266 hangs.


https://user-images.githubusercontent.com/80947240/120367633-b1507f80-c343-11eb-94fe-06323df313c6.mp4


