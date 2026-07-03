#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QVector>
#include <QtTextToSpeech/QTextToSpeech>

// Структура для хранения данных о человеке
struct Person {
    QString name;
    QPixmap photo;
    Person(QString n, QPixmap p) : name(n), photo(p) {}
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event) override; // Для отрисовки фона

private slots:
    void onNextWordButtonClicked();
    void updateRhymeWord();

private:
    Ui::MainWindow *ui;

    // ДОБАВЬТЕ ЭТУ СТРОКУ (исправляет вашу ошибку):
    QVector<QString> loadRhymesFromFolder(const QString &folderPath);

    // Остальные методы
    QList<Person> loadPhotos(const QString &folderPath);
    void displayPhotosInCircle(const QList<Person> &p);
    void highlightCurrentPerson();
    void removeCurrentPerson();
    void animateRemoval(QLabel *l);
    void displayWinner();
    void playSoundEffect(const QString &path);

    // Переменные
    QString basePath;
    QVector<QString> allRhymes;
    QList<Person> persons;
    QList<QLabel*> photoLabels;
    QTimer *rhymeTimer;
    QStringList rhymeWords;
    int currentIndex;
    int currentWordIndex;
    bool rhymeRunning;
    QLabel *highlightedLabel;
    QLabel *currentRhymeWordLabel;
    QTextToSpeech *speech;
};

#endif