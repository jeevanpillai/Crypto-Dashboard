# Crypto-Dashboard
A tiny OLED display used to display real-time information on my Cryptocurrency assets. Data obtained from Luno and Binance API

![20210602_004443](https://user-images.githubusercontent.com/80947240/120367656-b7466080-c343-11eb-8219-de9d89befa19.jpg)


This project is powered by an NodeMCU ESP8266 12-E Dev Board, it uses a TZT Real OLED Display measuring 3.12" with a dimension of 256x64 for its display. The display is controlled by the amazing [U8G2 Monochrome display library from Olikraus](https://github.com/olikraus/u8g2/). The time is kept in sync with an `NTPClient`. Below the time display is a scrolling window displaying the current prices of each asset held in my wallets, and in brackets, their percentage change over the last 8 hours (or as defined in the `price_change_delay` variable). Below that are the net wallet value of both exchanges along with their percentage change over the last x hours. NAV, or net asset value, displays the combined wallet values, with percentage of profit in brackets. All values are denoted in Malaysian Ringgit.

## How does it work?
1. Upon startup, it connects to the local WiFi Network.
2. It then reads asset balances from Luno and Binance
3. The prices of said assets are then requested, along with their prices 8 hours ago (or as defined in the `price_change_delay` variable)
4. Prices are updated every 5 minutes.

## Difficulty with the Binance API
Obtaining ticker data from Binance and Luno is easy enough, as these GET requests do not require Authentication. Obtaining user data such as wallet balances does require authentication. Authentication for Luno uses a Base64 encoded string with the format `Basic Base64("API_KEY:API_SECRET")`. This is the passed with the `Authorization` header in the HTTPS request.

>It is important to note that API Keys may compromise your wallet integrity. As such, for projects that require read-only information such as this, it's highly recommended that the API Keys generated are generated as READ-ONLY

For Binance however, things are more complicated. The API_KEY is first passed in the header with `X-MBX-APIKEY`. Then the query string needs to be generated with the mandatory `timestamp` parameter, along with the optional `recvWindow` parameter which denotes the duration of validity of the request. The `timestamp` parameter should not be more than 1000ms lesser than the server time or the request is rejected. The query string will end up looking something like:

>`timestamp=1621959833&recvWindow=20000`

This string is then hashed with the SHA256 HMAC algorithm using the `API_SECRET` as the Secret Key and the query string as the text input. The obtained 64-byte hex signature is then appended to the end of the query string with the `signature` parameter, resulting in a final query string of:

>`timestamp=1621959833&recvWindow=20000&signature=<SHA256 HMAC Signature>`

Big thanks to [Seeed-Studio's mbedtls module](https://github.com/Seeed-Studio/Seeed_Arduino_mbedtls) than made the SHA256 HMAC Signature generation possible.

## Limitations
1. To calculate the profit percentage in the NAV line, the initial investment amount is hardcoded. I plan to add the ability to update this value over WiFi soon.
2. Poor memory usage; Initialization takes on average 45s to 50s, during this process execution has a small chance of failing as the ESP8266 hangs.


https://user-images.githubusercontent.com/80947240/120367633-b1507f80-c343-11eb-94fe-06323df313c6.mp4


