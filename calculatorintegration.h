// calculatorintegration.h
#ifndef CALCULATORINTEGRATION_H
#define CALCULATORINTEGRATION_H

#include <QString>

struct CalculationSuggestion
{
    bool valid = false;
    QString expression;
    QString result;
    QString replacement;
};

class CalculatorIntegration
{
public:
    CalculationSuggestion suggestionForText(const QString &plainText) const;
    CalculationSuggestion evaluateExpression(const QString &expression) const;
};

#endif // CALCULATORINTEGRATION_H
