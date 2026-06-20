// templatemanager.cpp
#include "templatemanager.h"

QList<NoteTemplate> TemplateManager::templates()
{
    return {
        {QStringLiteral("Daily Journal"),
         QStringLiteral("Daily Journal"),
         QStringLiteral("# Journal\n\nDate:\n\nHighlights:\n- \n\nNotes:\n"),
         {QStringLiteral("Personal")}},
        {QStringLiteral("Meeting Notes"),
         QStringLiteral("Meeting Notes"),
         QStringLiteral("# Meeting Notes\n\nAttendees:\n\nAgenda:\n- \n\nDecisions:\n- \n\nNext Actions:\n- [ ] "),
         {QStringLiteral("Work")}},
        {QStringLiteral("Expense Tracker"),
         QStringLiteral("Expense Tracker"),
         QStringLiteral("# Expense Tracker\n\n| Item | Amount | Notes |\n| --- | --- | --- |\n|  |  |  |\n\n#Finance"),
         {QStringLiteral("Finance")}},
        {QStringLiteral("Loan Tracker"),
         QStringLiteral("Loan Tracker"),
         QStringLiteral("# Loan Tracker\n\nPrincipal:\nRate:\nTerm:\nEMI:\n\n[[Calculator Notes]]\n\n#Finance"),
         {QStringLiteral("Finance")}},
        {QStringLiteral("Meter Reading"),
         QStringLiteral("Meter Reading"),
         QStringLiteral("# Meter Reading\n\nPrevious:\nCurrent:\nUnits:\nBill:\n\n[[Electricity Notes]]"),
         {QStringLiteral("Personal")}},
        {QStringLiteral("Calculation Notes"),
         QStringLiteral("Calculation Notes"),
         QStringLiteral("# Calculation Notes\n\nType an expression like $1200*5 and press Enter."),
         {QStringLiteral("Finance"), QStringLiteral("Ideas")}}
    };
}

NoteTemplate TemplateManager::templateNamed(const QString &name)
{
    for (const NoteTemplate &noteTemplate : templates()) {
        if (noteTemplate.name.compare(name, Qt::CaseInsensitive) == 0)
            return noteTemplate;
    }
    return {};
}
