#pragma once

#include <QCompleter>

class QLineEdit;
class QString;
class QStringList;

class DelimitedCompleter : public QCompleter {
    Q_OBJECT

public:
    explicit DelimitedCompleter(QLineEdit* parent, char delimiter);
    explicit DelimitedCompleter(QLineEdit* parent, char delimiter, QAbstractItemModel* model);
    explicit DelimitedCompleter(QLineEdit* parent, char delimiter, const QStringList& list);

    QString pathFromIndex(const QModelIndex& index) const;
    QStringList splitPath(const QString& path) const;

private:
    void connectSignals();

    char delimiter;
    mutable int cursor_pos{-1};

private slots:
    void onActivated(const QString& text);
    void onCursorPositionChanged(int old_pos, int new_pos);
};
