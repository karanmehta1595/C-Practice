// calculatorintegration.cpp
#include "calculatorintegration.h"
#include "calculatorengine.h"


CalculationSuggestion CalculatorIntegration::suggestionForText(const QString &plainText) const
{
    const QStringList lines = plainText.split(QLatin1Char('\n'));
    for (int i = lines.size() - 1; i >= 0; --i) {
        const QString line = lines.at(i).trimmed();
        if (line.isEmpty())
            continue;
        return evaluateExpression(line);
    }
    return {};
}

CalculationSuggestion CalculatorIntegration::evaluateExpression(const QString &expression) const
{
    CalculationSuggestion suggestion;
    QString trimmed = expression.trimmed();
    if (trimmed.isEmpty()) return suggestion;

    QString result = CalculatorEngine::instance()->evaluateExpression(trimmed);
    
    if (result != trimmed && result != QStringLiteral("Error")) {
        suggestion.valid = true;
        suggestion.expression = trimmed;
        suggestion.result = result;
        suggestion.replacement = trimmed + QStringLiteral(" = ") + result;
    }
    return suggestion;
}


