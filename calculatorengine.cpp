#include "calculatorengine.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

CalculatorEngine* CalculatorEngine::s_instance = nullptr;

CalculatorEngine::CalculatorEngine(QObject *parent) : QObject(parent) {
    s_instance = this;
}

CalculatorEngine* CalculatorEngine::instance() {
    if (!s_instance) s_instance = new CalculatorEngine();
    return s_instance;
}

QString CalculatorEngine::detectCurrency(const QString &text) {
    QStringList symbols = {"₹", "$", "€", "£", "¥", "₩", "₽", "د.إ", "﷼", "৳", "₪", "₦"};
    for (const QString &sym : symbols) {
        if (text.contains(sym)) return sym;
    }
    return "";
}

QString CalculatorEngine::evaluateExpression(const QString &expression) {
    QString cleanExpr = expression;
    QString currencySymbol = detectCurrency(cleanExpr);
    
    // Strip ALL known currency symbols to handle mixed strings (e.g. 2000*$12000)
    QStringList symbols = {"₹", "$", "€", "£", "¥", "₩", "₽", "د.إ", "﷼", "৳", "₪", "₦"};
    for (const QString &sym : symbols) {
        cleanExpr.remove(sym);
    }
    cleanExpr.remove(',').remove(' ');
    
    // Normalize multiplication sign
    cleanExpr.replace("x", "*", Qt::CaseInsensitive);
    
    double result = 0;
    
    // Parse Percentage operations (e.g. 15000+10% or 100*10%)
    QRegularExpression percRe("^(-?[\\d.]+)([+\\-*/])([\\d.]+)%$");
    QRegularExpressionMatch percM = percRe.match(cleanExpr);
    
    if (percM.hasMatch()) {
        double base = percM.captured(1).toDouble();
        QString op = percM.captured(2);
        double percent = percM.captured(3).toDouble();
        
        if (op == "+" || op == "-") {
            double percentVal = calculatePercentage(base, percent);
            result = calculateOperation(base, op, percentVal);
        } else {
            double percentVal = percent / 100.0;
            result = calculateOperation(base, op, percentVal);
        }
    } else {
        // Parse standard A op B expressions (e.g. 2000*12)
        QRegularExpression re("^(-?[\\d.]+)([+\\-*/])(-?[\\d.]+)$");
        QRegularExpressionMatch m = re.match(cleanExpr);
        
        if (m.hasMatch()) {
            double a = m.captured(1).toDouble();
            QString op = m.captured(2);
            double b = m.captured(3).toDouble();
            result = calculateOperation(a, op, b);
        } else {
            // Fallback: try parsing as a flat number
            bool ok;
            result = cleanExpr.toDouble(&ok);
            if (!ok) return expression;
        }
    }
    
    return formatResult(result, currencySymbol);
}

QString CalculatorEngine::formatResult(double v, const QString &currencySymbol) {
    if (std::isnan(v)) return "Error";
    
    QString resStr;
    if (!currencySymbol.isEmpty()) {
        resStr = QString::number(v, 'f', 2); // Strict 2 decimals for currency
    } else {
        resStr = QString::number(v, 'f', 6); // Up to 6 decimals for precision
    }
    
    QStringList parts = resStr.split('.');
    QString intPart = parts[0];
    
    // Dynamic Number Formatting: Indian (2 digits) vs International (3 digits)
    int groupSize = (currencySymbol == "₹" || currencySymbol == "৳" || currencySymbol.isEmpty()) ? 2 : 3;
    
    int insertPos = intPart.length() - 3;
    if (insertPos > 0 && intPart[insertPos - 1] != '-') {
        intPart.insert(insertPos, ',');
        insertPos -= groupSize;
        while (insertPos > 0) {
            if (intPart[insertPos - 1] == '-') break;
            intPart.insert(insertPos, ',');
            insertPos -= groupSize;
        }
    }
    
    if (parts.size() > 1) {
        QString decPart = parts[1];
        if (currencySymbol.isEmpty()) {
            // Strip trailing zeros for non-currency mathematical results
            while (decPart.endsWith('0')) decPart.chop(1);
        }
        if (!decPart.isEmpty()) {
            resStr = intPart + "." + decPart;
        } else {
            resStr = intPart;
        }
    } else {
        resStr = intPart;
    }
    
    return currencySymbol + resStr;
}

double CalculatorEngine::calculateOperation(double a, const QString &op, double b) {
    if (op == "+") return a + b;
    if (op == "-") return a - b;
    if (op == "*" || op == "x") return a * b;
    if (op == "/") return b != 0 ? a / b : NAN;
    if (op == "x^y") return std::pow(a, b);
    if (op == "y√x") return b != 0 ? std::pow(a, 1.0 / b) : NAN;
    if (op == "|mod|") return std::fmod(a, b);
    return 0;
}

double CalculatorEngine::calculatePercentage(double base, double percentage) {
    return (base * percentage) / 100.0;
}

void CalculatorEngine::setExchangeRates(const QMap<QString, double> &rates) {
    m_exchangeRates = rates;
}

double CalculatorEngine::convertCurrency(double amount, double fromRate, double toRate) {
    if (fromRate == 0) return 0;
    return (amount / fromRate) * toRate;
}

QJsonObject CalculatorEngine::calculateEMI(double principal, double annualRate, int months) {
    QJsonObject res;
    if (principal <= 0 || annualRate <= 0 || months <= 0) {
        res["error"] = true;
        return res;
    }
    double r = annualRate / (12.0 * 100.0);
    double emi = principal * r * std::pow(1 + r, months) / (std::pow(1 + r, months) - 1);
    double totalAmt = emi * months;
    double totalInt = totalAmt - principal;

    res["error"] = false;
    res["emi"] = emi;
    res["totalAmount"] = totalAmt;
    res["totalInterest"] = totalInt;
    res["principalPercentage"] = (principal / totalAmt) * 100.0;
    res["interestPercentage"] = (totalInt / totalAmt) * 100.0;
    return res;
}

QJsonObject CalculatorEngine::calculateGST(double amount, double rate, bool exclusive) {
    QJsonObject res;
    if (amount <= 0) {
        res["error"] = true;
        return res;
    }
    double baseAmt = 0, gstAmt = 0, totalAmt = 0;
    if (exclusive) {
        baseAmt = amount;
        gstAmt = amount * rate / 100.0;
        totalAmt = baseAmt + gstAmt;
    } else {
        totalAmt = amount;
        baseAmt = amount * 100.0 / (100.0 + rate);
        gstAmt = totalAmt - baseAmt;
    }
    res["error"] = false;
    res["baseAmount"] = baseAmt;
    res["gstAmount"] = gstAmt;
    res["cgst"] = gstAmt / 2.0;
    res["sgst"] = gstAmt / 2.0;
    res["igst"] = gstAmt;
    res["totalAmount"] = totalAmt;
    return res;
}

double CalculatorEngine::calculateArea(const QString &shape, const QList<double> &v) {
    if (shape == "Square") return v[0]*v[0];
    if (shape == "Rectangle") return v[0]*v[1];
    if (shape == "Triangle") return 0.5*v[0]*v[1];
    if (shape == "Circle") return M_PI*v[0]*v[0];
    if (shape == "Parallelogram") return v[0]*v[1];
    if (shape == "Trapezoid") return 0.5*(v[0]+v[1])*v[2];
    if (shape == "Ellipse") return M_PI*v[0]*v[1];
    if (shape == "Rhombus") return 0.5*v[0]*v[1];
    if (shape == "Regular Hexagon") return (3.0*std::sqrt(3.0)/2.0)*v[0]*v[0];
    if (shape == "Regular Pentagon") return (std::sqrt(5.0*(5.0+2.0*std::sqrt(5.0)))/4.0)*v[0]*v[0];
    if (shape == "Sector") return 0.5*v[0]*v[0]*(v[1]*M_PI/180.0);
    if (shape == "Circle Circumference") return 2.0*M_PI*v[0];
    if (shape == "Cube") return v[0]*v[0]*v[0];
    if (shape == "Cuboid/Box") return v[0]*v[1]*v[2];
    if (shape == "Cylinder") return M_PI*v[0]*v[0]*v[1];
    if (shape == "Sphere") return (4.0/3.0)*M_PI*v[0]*v[0]*v[0];
    if (shape == "Cone") return (1.0/3.0)*M_PI*v[0]*v[0]*v[1];
    if (shape == "Pyramid (square)") return (1.0/3.0)*v[0]*v[0]*v[1];
    if (shape == "Hemisphere") return (2.0/3.0)*M_PI*v[0]*v[0]*v[0];
    if (shape == "Torus") return 2.0*M_PI*M_PI*v[0]*v[1]*v[1];
    if (shape == "Prism (triangle)") return 0.5*v[0]*v[1]*v[2];
    if (shape == "Sphere Surface") return 4.0*M_PI*v[0]*v[0];
    if (shape == "Cylinder Surface") return 2.0*M_PI*v[0]*(v[0]+v[1]);
    if (shape == "Cone Surface") return M_PI*v[0]*(v[0]+v[1]);
    return 0;
}

double CalculatorEngine::calculateVolume(const QString &shape, const QList<double> &v) {
    return calculateArea(shape, v); // Using shared dispatcher for now
}

QList<UnitGroup> CalculatorEngine::getUnitGroups() const {
    return {
        {"Length", {"Metre","Kilometre","Mile","Yard","Foot","Inch","Centimetre","Millimetre","Nautical Mile"}, {1.0, 1000.0, 1609.344, 0.9144, 0.3048, 0.0254, 0.01, 0.001, 1852.0}},
        {"Mass / Weight", {"Kilogram","Gram","Milligram","Tonne","Pound","Ounce","Stone"}, {1.0, 0.001, 0.000001, 1000.0, 0.453592, 0.0283495, 6.35029}},
        {"Temperature", {"Celsius","Fahrenheit","Kelvin"}, {1.0, 1.0, 1.0}},
        {"Area", {"m²","km²","Hectare","Acre","ft²","inch²","Mile²"}, {1.0, 1e6, 10000.0, 4046.86, 0.092903, 0.000645, 2.59e6}},
        {"Volume", {"Litre","Millilitre","Cubic metre","Gallon (US)","Pint (US)","Cup","Fluid oz"}, {1.0, 0.001, 1000.0, 3.78541, 0.473176, 0.236588, 0.0295735}},
        {"Speed", {"m/s","km/h","mph","Knot","ft/s"}, {1.0, 0.277778, 0.44704, 0.514444, 0.3048}},
        {"Time", {"Second","Minute","Hour","Day","Week","Month (avg)","Year (avg)"}, {1.0, 60.0, 3600.0, 86400.0, 604800.0, 2629800.0, 31557600.0}},
        {"Energy", {"Joule","Kilojoule","Calorie","kcal","kWh","eV"}, {1.0, 1000.0, 4.184, 4184.0, 3.6e6, 1.60218e-19}},
        {"Power", {"Watt","Kilowatt","Megawatt","HP","BTU/hr"}, {1.0, 1000.0, 1e6, 745.7, 0.29307}},
        {"Pressure", {"Pascal","kPa","Bar","Atm","PSI","mmHg"}, {1.0, 1000.0, 100000.0, 101325.0, 6894.76, 133.322}},
        {"Data", {"Byte","KB","MB","GB","TB","Bit"}, {1.0, 1024.0, 1048576.0, 1073741824.0, 1099511627776.0, 0.125}},
        {"Angle", {"Degree","Radian","Gradian"}, {1.0, 57.2958, 0.9}},
        {"Fuel Economy", {"L/100km","mpg (US)","km/L"}, {1.0, 235.215, 100.0}}
    };
}

double CalculatorEngine::convertUnit(const QString &groupName, double val, int fromIndex, int toIndex) {
    if (groupName == "Temperature") {
        double celsius = val;
        if (fromIndex == 1) celsius = (val - 32.0) / 1.8;
        else if (fromIndex == 2) celsius = val - 273.15;
        if (toIndex == 0) return celsius;
        if (toIndex == 1) return celsius * 1.8 + 32.0;
        return celsius + 273.15;
    }
    QList<UnitGroup> groups = getUnitGroups();
    for (const auto &g : groups) {
        if (g.name == groupName) {
            double base = val * g.toBase[fromIndex];
            return base / g.toBase[toIndex];
        }
    }
    return 0.0;
}

double CalculatorEngine::convertCurrency(double amount, const QString& fromCode, const QString& toCode) {
    double from = m_exchangeRates.value(fromCode, 1.0);
    double to = m_exchangeRates.value(toCode, 1.0);
    return convertCurrency(amount, from, to);
}