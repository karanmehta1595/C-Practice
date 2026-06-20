#include "currencyservice.h"
#include "calculatorengine.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

CurrencyService* CurrencyService::s_instance = nullptr;

CurrencyService::CurrencyService(QObject *parent) : QObject(parent) {
    s_instance = this;
    m_netManager = new QNetworkAccessManager(this);
}

CurrencyService* CurrencyService::instance() {
    if (!s_instance) s_instance = new CurrencyService();
    return s_instance;
}

QString CurrencyService::getLastRateUpdate() const { return m_lastRateUpdate; }
QString CurrencyService::getRateSource() const { return m_rateSource; }

void CurrencyService::fetchLiveRates() {
    QNetworkReply *reply = m_netManager->get(
        QNetworkRequest(QUrl("https://open.er-api.com/v6/latest/USD")));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QByteArray data = reply->readAll();
        QJsonObject root = QJsonDocument::fromJson(data).object();
        m_lastRateUpdate = QDateTime::currentDateTime().toString("dd MMM yyyy hh:mm AP");
        
        if (root.contains("rates")) {
            QJsonObject rates = root["rates"].toObject();
            QMap<QString, double> exchangeRates;
            static const QMap<QString,QString> codeMap = {
                {"USD","USD ($)"},  {"INR","INR (₹)"}, {"EUR","EUR (€)"}, {"GBP","GBP (£)"},
                {"JPY","JPY (¥)"},  {"CNY","CNY (¥)"}, {"KRW","KRW (₩)"},{"RUB","RUB (₽)"},
                {"TRY","TRY (₺)"},  {"VND","VND (₫)"}, {"THB","THB (฿)"},{"PHP","PHP (₱)"},
                {"BDT","BDT (৳)"},  {"ILS","ILS (₪)"}, {"NGN","NGN (₦)"},{"AUD","AUD ($)"},
                {"CAD","CAD ($)"},  {"NZD","NZD (NZ$)"},{"SGD","SGD (S$)"},{"HKD","HKD (HK$)"},
                {"TWD","TWD (NT$)"},{"PKR","PKR (₨)"}, {"NPR","NPR (₨)"},{"LKR","LKR (₨)"},
                {"AED","AED (د.إ)"},{"SAR","SAR (﷼)"},{"QAR","QAR (﷼)"},{"OMR","OMR (﷼)"},
                {"KWD","KWD (KD)"}, {"BHD","BHD (BD)"},{"JOD","JOD (JD)"},{"MYR","MYR (RM)"},
                {"IDR","IDR (Rp)"}, {"KHR","KHR (៛)"}, {"LAK","LAK (₭)"},{"MNT","MNT (₮)"},
                {"KZT","KZT (₸)"},  {"AZN","AZN (₼)"}, {"GEL","GEL (₾)"},{"UAH","UAH (₴)"},
                {"BYN","BYN (Br)"}, {"RON","RON (lei)"},{"BGN","BGN (лв)"},{"RSD","RSD (дин)"},
                {"ISK","ISK (kr)"}, {"KES","KES (KSh)"},{"UGX","UGX (USh)"},{"TZS","TZS (TSh)"},
                {"ETB","ETB (Br)"}, {"GHS","GHS (₵)"}, {"MAD","MAD (د.م.)"},{"DZD","DZD (دج)"},
                {"EGP","EGP (£)"},  {"TND","TND (د.ت)"},{"BRL","BRL (R$)"},{"ARS","ARS ($)"},
                {"CLP","CLP ($)"},  {"COP","COP ($)"}, {"UYU","UYU ($U)"},{"PEN","PEN (S/)"},
                {"BOB","BOB (Bs.)"},{"PYG","PYG (₲)"}, {"CRC","CRC (₡)"}, {"GTQ","GTQ (Q)"},
                {"HNL","HNL (L)"},  {"NIO","NIO (C$)"},{"DOP","DOP ($)"}, {"JMD","JMD ($)"},
                {"MXN","MXN ($)"},  {"ZAR","ZAR (R)"}, {"CHF","CHF (CHF)"},{"SEK","SEK (kr)"},
                {"NOK","NOK (kr)"}, {"DKK","DKK (kr)"},{"PLN","PLN (zł)"},{"CZK","CZK (Kč)"},
                {"HUF","HUF (Ft)"}, {"TTD","TTD (TT$)"},{"BSD","BSD ($)"},{"BBD","BBD ($)"}
            };

            for (const QString &code : rates.keys()) {
                QString dn = codeMap.value(code, code);
                exchangeRates[dn] = rates[code].toDouble();
            }
            CalculatorEngine::instance()->setExchangeRates(exchangeRates);
            emit ratesUpdated();
        }
        reply->deleteLater();
    });
}