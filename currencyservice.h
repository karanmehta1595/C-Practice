#ifndef CURRENCYSERVICE_H
#define CURRENCYSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QMap>
#include <QString>

class CurrencyService : public QObject {
    Q_OBJECT
public:
    explicit CurrencyService(QObject *parent = nullptr);
    static CurrencyService* instance();

    void fetchLiveRates();
    QString getLastRateUpdate() const;
    QString getRateSource() const;

signals:
    void ratesUpdated();

private:
    static CurrencyService* s_instance;
    QNetworkAccessManager *m_netManager;
    QString m_lastRateUpdate;
    QString m_rateSource = "ER-API";
};

#endif // CURRENCYSERVICE_H