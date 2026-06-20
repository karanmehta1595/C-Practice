#ifndef CALCULATORENGINE_H
#define CALCULATORENGINE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QJsonObject>
#include <cmath>

struct UnitGroup {
    QString name;
    QStringList units;
    QList<double> toBase;
};

class CalculatorEngine : public QObject {
    Q_OBJECT
public:
    explicit CalculatorEngine(QObject *parent = nullptr);
    static CalculatorEngine* instance();

    // ─────────────────────────────────────────────
    //  CORE API (Used by Calculator & Notes)
    // ─────────────────────────────────────────────
    QString evaluateExpression(const QString &expression);
    QString formatResult(double v, const QString &currencySymbol = "");

    // Basic Math
    double calculateOperation(double a, const QString &op, double b);
    double calculatePercentage(double base, double percentage);

    // Currency
    void setExchangeRates(const QMap<QString, double> &rates);
    QString detectCurrency(const QString &text);
    double convertCurrency(double amount, double fromRate, double toRate);
    double convertCurrency(double amount, const QString& fromCode, const QString& toCode);

    // Units
    QList<UnitGroup> getUnitGroups() const;
    double convertUnit(const QString &groupName, double val, int fromIndex, int toIndex);

    // Business Logic API
    QJsonObject calculateEMI(double principal, double annualRate, int months);
    QJsonObject calculateGST(double amount, double rate, bool exclusive);

    // Geometry API
    double calculateArea(const QString &shape, const QList<double> &v);
    double calculateVolume(const QString &shape, const QList<double> &v);

private:
    static CalculatorEngine* s_instance;
    QMap<QString, double> m_exchangeRates;
};

#endif // CALCULATORENGINE_H