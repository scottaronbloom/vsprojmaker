// The MIT License( MIT )
//
// Copyright( c ) 2020-2021 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "Settings.h"
#include "DebugTarget.h"
#include "VSProjectMaker.h"
#include "DirInfo.h"

#include <QApplication>
#include <QStandardItem>
#include <QString>
#include <QStringList>
#include <QList>
#include <QSet>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QProgressDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QProcess>

namespace NVSProjectMaker
{
    CSettings::CSettings() :
        fResults( std::make_shared< SSourceFileResults >() )
    {
        NVSProjectMaker::registerTypes();

        registerSettings();
        loadSettings();
    }

    CSettings::CSettings( const QString & fileName ) :
        fResults( std::make_shared< SSourceFileResults >() )
    {
        NVSProjectMaker::registerTypes();

        registerSettings();
        loadSettings( fileName );
    }

    CSettings::~CSettings()
    {
        saveSettings();
    }

    QString CSettings::fileName() const
    {
        if ( !fSettingsFile )
            return QString();
        auto fname = fSettingsFile->fileName();
        if ( fname.startsWith( "\\HKEY" ) )
            return QString();
        return fname;
    }

    bool CSettings::saveSettings()
    {
        if ( !fSettingsFile )
            return false;

        for ( auto && ii : fSettings )
            fSettingsFile->setValue( ii.first, *(ii.second) );

        return true;
    }

    void CSettings::loadSettings()
    {
        loadSettings( QString() );
    }

    void CSettings::setFileName( const QString & fileName, bool andSave )
    {
        if ( fileName.isEmpty() )
            fSettingsFile = std::make_unique< QSettings >();
        else
            fSettingsFile = std::make_unique< QSettings >( fileName, QSettings::IniFormat );
        if ( andSave )
            saveSettings();
    }

    void CSettings::incProgress( QProgressDialog * progress ) const
    {
        if ( !progress )
            return;

        progress->setValue( progress->value() + 1 );
        //if ( progress->value() >= 100 )
        //    progress->setValue( 1 );
        qApp->processEvents();
    }

    bool CSettings::isBuildDir( const QDir & relToDir, const QDir & dir ) const
    {
        bool retVal = QFileInfo( relToDir.absoluteFilePath( dir.filePath( "subdir.mk" ) ) ).exists();
        retVal = retVal || QFileInfo( relToDir.absoluteFilePath( dir.filePath( "Makefile" ) ) ).exists();
        retVal = retVal || getBuildDirs().contains( dir.path() );
        return retVal;
    }

    bool CSettings::isInclDir( const QDir & relToDir, const QDir & dir ) const
    {
        bool retVal = dir.dirName() == "incl";
        retVal = retVal || getInclDirs().contains( dir.path() );
        if ( !retVal )
        {
            QDirIterator di( relToDir.absoluteFilePath( dir.path() ), { "*.h", "*.hh", "*.hpp" }, QDir::Files );
            retVal = di.hasNext();
        }
        return retVal;
    }

    QList< QPair< QString, bool > > CSettings::getExecutables( const QDir & dir ) const
    {
        auto execNames = getExecNames();
        auto pos = execNames.find( dir.path() );
        if ( pos != execNames.end() )
            return pos.value();
        return {};
    }

    QStringList CSettings::addInclDirs( const QStringList & inclDirs )
    {
        auto current = getInclDirs();
        for ( auto && ii : inclDirs )
        {
            if ( !current.contains( ii ) )
                current << ii;
        }

        return current;
    }

    QString CSettings::getIncludeDirs() const
    {
        QStringList retVal;

        if ( !getSourceDir().has_value() )
            return QString();

        QDir sourceDir( getSourceDir().value() );
        auto inclDirs = getSelectedInclDirs();
        for ( auto && ii : inclDirs )
        {
            auto curr = cleanUp( sourceDir.absoluteFilePath( ii ) );
            retVal << curr;
        }

        auto qtDirs = getSelectedQtDirs();
        auto qtInclDir = QDir( getQtDir() );
        qtInclDir.cd( "include" );
        retVal << qtInclDir.absolutePath();
        for ( auto && ii : qtDirs )
        {
            retVal << qtInclDir.absoluteFilePath( ii );
        }

        return retVal.join( ";" );
    }

    QStringList CSettings::getCmakeArgs() const
    {
        QStringList retVal;

        auto whichGenerator = getGenerator();

        auto pos = whichGenerator.indexOf( "=" );
        if ( pos != -1 )
            whichGenerator = whichGenerator.left( pos );

        whichGenerator.replace( "[arch]", "Win64" );
        whichGenerator = whichGenerator.trimmed();

        retVal << "-G" << whichGenerator << ".";
        return retVal;
    }

    void CSettings::generate( QProgressDialog * progress, QWidget * parent, const std::function< void( const QString & msg ) > & logit ) const
    {
        if ( !getBuildDir().has_value() )
            return;
        if ( !getSourceDir().has_value() )
            return;

        auto dirs = generateTopLevelFiles( progress, logit, parent );

        if ( progress )
            progress->setRange( 0, static_cast<int>( dirs.size() ) );
        logit( QObject::tr( "============================================" ) );
        logit( QObject::tr( "Generating CMake Files" ) );
        if ( progress )
        {
            progress->setLabelText( QObject::tr( "Generating CMake Files" ) );
            progress->adjustSize();
            qApp->processEvents();
        }

        auto inclDirs = getIncludeDirs();
        int currDir = 0;
        for ( auto && ii : dirs )
        {
            if ( progress )
            {
                progress->setValue( currDir++ );
                if ( progress->wasCanceled() )
                    break;
            }
        }
        
        for ( auto && ii : dirs )
        {
            if ( progress )
            {
                progress->setValue( currDir++ );
                if ( progress->wasCanceled() )
                    break;
            }
            ii->writeCMakeFile( parent, getBuildDir().value() );
            ii->writePropSheet( parent, getSourceDir().value(), getBuildDir().value(), inclDirs );
            ii->createDebugProjects( parent, getBuildDir().value() );
        }
    }

    QMap< QString, QString > CSettings::getVarMap() const
    {
        static QMap< QString, QString > map =
        {
              { "CLIENTDIR", QDir( getClientDir() ).absolutePath() }
            , { "SOURCEDIR", getSourceDir().value() }
            , { "BUILDDIR", getBuildDir().value() }
            , { "PRODDIR", QDir( getProdDir() ).absolutePath() }
        };

        return map;
    }

    QList < NVSProjectMaker::SDebugTarget > CSettings::getDebugCommandsForSourceDir( const QString & inSourcePath ) const
    {
        QList < NVSProjectMaker::SDebugTarget > retVal;

        auto sourceDirPath = getSourceDir();
        if ( !sourceDirPath.has_value() )
            return {};
        QDir sourceDir( sourceDirPath.value() );
        sourceDir.cdUp();
        auto cmds = getDebugCommands();
        for ( auto && curr : cmds )
        {
            //( QStringList() << "Source Dir" << "Name" << "Command" << "Args" << "Work Dir" << "EnvVars" )

            curr.cleanUp( this );

            if ( sourceDir.absoluteFilePath( curr.fSourceDir ) == inSourcePath )
            {
                retVal << curr;
            }
        }
        return retVal;
    }

    QStringList CSettings::getCustomBuildsForSourceDir( const QString & inSourcePath ) const
    {
        auto sourceDirPath = getSourceDir();
        if ( !sourceDirPath.has_value() )
            return {};

        QDir sourceDir( sourceDirPath.value() );
        auto srcPath = sourceDir.relativeFilePath( inSourcePath );

        auto buildDirPath = getBuildDir();
        if ( !buildDirPath.has_value() )
            return {};
        QDir buildDir( buildDirPath.value() );
        buildDir.cdUp();
        buildDir.cdUp();

        auto customBuilds = getCustomBuilds();
        QStringList retVal;
        for ( auto && ii : customBuilds )
        {
            auto currBuildDirPath = buildDir.absoluteFilePath( ii.first );
            auto currRelBuildDir = QDir( buildDirPath.value() ).relativeFilePath( currBuildDirPath );
            if ( currRelBuildDir == srcPath )
                retVal << ii.second;
        }
        return retVal;
    }

    std::list< std::shared_ptr< NVSProjectMaker::SDirInfo > > CSettings::generateTopLevelFiles( QProgressDialog * progress, const std::function< void( const QString & msg ) > & logit, QWidget * parent ) const
    {
        NVSProjectMaker::readResourceFile( parent, QString( ":/resources/Project.cmake" ),
                                           [this, parent]( QString & text )
        {
            QString outFile = QDir( getBuildDir().value() ).absoluteFilePath( "Project.cmake" );
            QFile fo( outFile );
            if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
            {
                QApplication::restoreOverrideCursor();
                QMessageBox::critical( parent, QObject::tr( "Error:" ), QObject::tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
                return;
            }
            QTextStream qts( &fo );
            qts << text;
            fo.close();
        }
        );

        auto topDirText = NVSProjectMaker::readResourceFile( parent, QString( ":/resources/%1.txt" ).arg( "topdir" ),
                                                             [this]( QString & text )
        {
            text.replace( "%CLIENT_NAME%", getClientName() );
            text.replace( "%ROOT_SOURCE_DIR%", getSourceDir().value() );
        }
        );

        QString outFile = QDir( getBuildDir().value() ).absoluteFilePath( "CMakeLists.txt" );
        QFile fo( outFile );
        if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
        {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical( parent, QObject::tr( "Error:" ), QObject::tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
            return {};
        }

        QTextStream qts( &fo );
        qts << topDirText;

        QStringList customTopBuilds = getCustomBuildsForSourceDir( QFileInfo( getSourceDir().value() ).absoluteFilePath() );
        if ( !customTopBuilds.isEmpty() )
        {
            for ( auto && ii : customTopBuilds )
            {
                qts << QString( "add_subdirectory( CustomBuild/%1 )\n" ).arg( ii );

                if ( !QDir( getBuildDir().value() ).mkpath( QString( "CustomBuild/%1" ).arg( ii ) ) )
                {
                    QApplication::restoreOverrideCursor();
                    QMessageBox::critical( parent, QObject::tr( "Error:" ), QObject::tr( "Error creating directory '%1" ).arg( QString( "CustomBuild/%1" ).arg( ii ) ) );
                    return {};
                }
                QString fileName = QString( "CustomBuild/%1/CMakeLists.txt" ).arg( ii );
                auto topDirCustomBuildTxt = NVSProjectMaker::readResourceFile( parent, QString( ":/resources/custombuilddir.txt" ),
                                                                               [ii, fileName, this, parent]( QString & text )
                {
                    text.replace( "%PROJECT_NAME%", ii );
                    text.replace( "%BUILD_DIR%", getBuildDir().value() );
                    text.replace( "%SOURCE_FILES%", QString() );
                    text.replace( "%HEADER_FILES%", QString() );
                    text.replace( "%UI_FILES%", QString() );
                    text.replace( "%QRC_FILES%", QString() );
                    text.replace( "%OTHER_FILES%", QString() );

                    QString outFile = QDir( getBuildDir().value() ).absoluteFilePath( fileName );
                    QFile fo( outFile );
                    if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
                    {
                        QApplication::restoreOverrideCursor();
                        QMessageBox::critical( parent, QObject::tr( "Error:" ), QObject::tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
                        return;
                    }
                    QTextStream qts( &fo );
                    qts << text;
                    qts << "\n\nset_target_properties( " << ii << " PROPERTIES FOLDER " << "\"Top Level Targets\"" << " )\n";
                    fo.close();
                }
                );
            }
            qts << "\n\n";
        }

        qApp->processEvents();
        logit( QObject::tr( "============================================" ) );
        logit( QObject::tr( "Finding Directories" ) );
        if ( progress )
        {
            progress->setLabelText( QObject::tr( "Finding Directories" ) );
            progress->adjustSize();
        }

        if ( !getParentOfPairDirectoriesMap( nullptr, progress ) )
        {
            QApplication::restoreOverrideCursor();
            return {};
        }

        auto dirs = getDirInfo( nullptr, progress );
        if ( progress && progress->wasCanceled() )
        {
            QApplication::restoreOverrideCursor();
            return {};
        }
        qts << "#######################################################\n"
            << "##### Sub Directories\n"
            ;

        if ( progress )
            progress->setRange( 0, static_cast<int>( dirs.size() ) );

        for ( auto && ii : dirs )
        {
            if ( progress && progress->wasCanceled() )
                return {};

            auto subDirs = ii->getSubDirs();
            for ( auto && jj : subDirs )
                qts << QString( "add_subdirectory( %1 )\n" ).arg( jj );
        }
        return dirs;
    }

    bool CSettings::getParentOfPairDirectoriesMap( std::shared_ptr< SSourceFileInfo > parent, QProgressDialog * progress ) const
    {
        auto && childList = parent ? parent->fChildren : fResults->fRootDir->fChildren;
        auto numRows = childList.size();
        if ( progress )
            progress->setRange( 0, static_cast<int>( numRows ) );
        QDir sourceDir( getSourceDir().value() );

        auto currNum = 0;
        for ( auto && curr : childList )
        {
            if ( progress )
                progress->setValue( currNum++ );
            qApp->processEvents();
            if ( progress && progress->wasCanceled() )
                return false;

            if ( !curr->fIsDir )
                continue;

            if ( !getParentOfPairDirectoriesMap( curr, progress ) )
                 return false;

            if ( !curr->isPairedInclSrcDir( getSourceDir().value() ) )
                continue;

            if ( parent )
                parent->fPairedChildDirectores.push_back( curr );
            else
                fResults->fRootDir->fPairedChildDirectores.push_back( curr );
        }
        if ( progress && progress->wasCanceled() )
            return false;
        return true;
    }

    std::list< std::shared_ptr< NVSProjectMaker::SDirInfo > > CSettings::getDirInfo( std::shared_ptr< SSourceFileInfo > parent, QProgressDialog * progress ) const
    {
        std::list< std::shared_ptr< NVSProjectMaker::SDirInfo > > retVal;

        if ( parent && !parent->fIsDir )
            return {};

        auto && childList = parent ? parent->fChildren : fResults->fRootDir->fChildren;
        auto numRows = childList.size();
        if ( progress )
            progress->setRange( 0, static_cast<int>( numRows ) );
        QDir sourceDir( getSourceDir().value() );

        auto currNum = 0;
        for( auto && curr : childList )
        {
            if ( progress )
                progress->setValue( currNum++ );
            qApp->processEvents();
            if ( progress && progress->wasCanceled() )
                return {};

            if ( !curr->fIsDir )
            {
                continue;
            }

            if ( curr->isPairedInclSrcDir( getSourceDir().value() ) )
                 continue;

            auto currInfo = std::make_shared< NVSProjectMaker::SDirInfo >( curr );
            currInfo->fExtraTargets = getCustomBuildsForSourceDir( QFileInfo( sourceDir.absoluteFilePath( currInfo->fRelToDir ) ).canonicalFilePath() );
            currInfo->fDebugCommands = getDebugCommandsForSourceDir( QFileInfo( sourceDir.absoluteFilePath( currInfo->fRelToDir ) ).canonicalFilePath() );

            if ( curr->isParentToPairedDirs( getSourceDir().value() ) )
            {
                currInfo->fExtraTargets  << getCustomBuildsForSourceDir( QFileInfo( sourceDir.absoluteFilePath( currInfo->fRelToDir + "/src" ) ).canonicalFilePath() );
                currInfo->fExtraTargets  << getCustomBuildsForSourceDir( QFileInfo( sourceDir.absoluteFilePath( currInfo->fRelToDir + "/incl" ) ).canonicalFilePath() );

                currInfo->fDebugCommands << getDebugCommandsForSourceDir( QFileInfo( sourceDir.absoluteFilePath( currInfo->fRelToDir + "/src" ) ).canonicalFilePath() );
                currInfo->fDebugCommands << getDebugCommandsForSourceDir( QFileInfo( sourceDir.absoluteFilePath( currInfo->fRelToDir + "/incl" ) ).canonicalFilePath() );
                retVal.push_back( currInfo );
            }
            else if ( currInfo->isValid() )
            {
                retVal.push_back( currInfo );
            }
            auto children = getDirInfo( curr, progress );
            retVal.insert( retVal.end(), children.begin(), children.end() );
        }
        if ( progress && progress->wasCanceled() )
            return {};

        return retVal;
    }

    QString CSettings::getClientName() const
    {
        if ( getClientDir().isEmpty() )
            return QString();

        QDir dir( getClientDir() );
        if ( !dir.exists() )
            return QString();

        return dir.dirName();
    }

    QString CSettings::cleanUp( const QString & str ) const
    {
        auto map = getVarMap();
        QString retVal = str;
        for ( auto && ii = map.cbegin(); ii != map.cend(); ++ii )
        {
            auto key = "<" + ii.key() + ">";
            auto value = ii.value();
            retVal = retVal.replace( key, value );
        }
        return retVal;
    }

    bool CSettings::loadSourceFiles( const QDir & sourceDir, const QString & dir, QProgressDialog * progress, std::shared_ptr< SSourceFileInfo  > parent, const std::function< void( const QString & msg ) > & logit )
    {
        QDir baseDir( dir );
        if ( !baseDir.exists() || !sourceDir.exists() )
            return false;

        auto relDirPath = sourceDir.relativeFilePath( dir );
        logit( relDirPath );
        if ( progress )
        {
            progress->setLabelText( QObject::tr( "Finding Source Files...\n%1" ).arg( relDirPath ) );
            progress->adjustSize();
            progress->setRange( 0, baseDir.count() );
            incProgress( progress );
        }

        QDirIterator iter( dir, QStringList(), QDir::Filter::AllDirs | QDir::Filter::Files | QDir::Filter::NoDotAndDotDot | QDir::Filter::Readable, QDirIterator::IteratorFlag::NoIteratorFlags );
        while ( iter.hasNext() )
        {
            incProgress( progress );
            if ( progress && progress->wasCanceled() )
                return true;

            auto curr = QFileInfo( iter.next() );
            relDirPath = sourceDir.relativeFilePath( curr.absoluteFilePath() );
            auto node = std::make_shared< SSourceFileInfo >();
            node->fIsDir = curr.isDir();
            node->fRelToDir = relDirPath;
            parent->fChildren.push_back( node );
            
            if ( !curr.isDir() )
            {
                fResults->fFiles++;
            }
            else
            {
                fResults->fDirs++;
                // determine if its a build dir, has an exec name (or names), or include path
                node->fIsBuildDir = CSettings::isBuildDir( sourceDir, relDirPath );
                node->fIsIncludeDir = CSettings::isInclDir( sourceDir, relDirPath );
                node->fExecutables = CSettings::getExecutables( relDirPath );

                QStringList execNames;
                for ( auto && ii : node->fExecutables )
                    execNames << ii.first;
                QStringList guiExecs;
                for ( auto && ii : node->fExecutables )
                    guiExecs << ( ii.second ? "Yes" : "No" );

                if ( node->fIsBuildDir )
                    fResults->fBuildDirs.push_back( relDirPath );
                if ( node->fIsIncludeDir )
                    fResults->fInclDirs.push_back( relDirPath );
                fResults->fExecutables << node->fExecutables;

                if ( loadSourceFiles( sourceDir, curr.absoluteFilePath(), progress, node, logit ) )
                    return true;
            }

        }
        return progress ? progress->wasCanceled() : false;
    }

    bool CSettings::loadSourceFiles( const QDir & sourceDir, const QString & dir, QProgressDialog * progress, const std::function< void( const QString & msg ) > & logit )
    {
        return loadSourceFiles( sourceDir, dir, progress, fResults->fRootDir, logit );
    }

    std::optional< QString > CSettings::getDir( const QString & relDir, bool relPath ) const
    {
        if ( relPath )
            return relDir;

        auto clientDirString = getClientDir();
        if ( clientDirString.isEmpty() )
            return {};

        QDir clientDir( clientDirString );
        if ( !clientDir.exists() )
            return {};

        if ( relDir.isEmpty() )
            return {};

        if ( !QFileInfo( clientDir.absoluteFilePath( relDir ) ).exists() )
            return {};

        return clientDir.absoluteFilePath( relDir );
    }

    std::optional< QString > CSettings::getBuildDir( bool relPath ) const
    {
        return getDir( getBuildRelDir(), relPath );
    }

    std::optional< QString > CSettings::getSourceDir( bool relPath ) const
    {
        return getDir( getSourceRelDir(), relPath );
    }


    bool CSettings::loadSettings( const QString & fileName )
    {
        saveSettings();
        setFileName( fileName, false );
        return loadData();
    }

    void CSettings::registerSettings()
    {
        ADD_SETTING_VALUE( QString, CMakePath );
        ADD_SETTING_VALUE( QString, Generator );
        ADD_SETTING_VALUE( QString, ClientDir );
        ADD_SETTING_VALUE( QString, SourceRelDir );
        ADD_SETTING_VALUE( QString, BuildRelDir );
        ADD_SETTING_VALUE( QString, QtDir );
        ADD_SETTING_VALUE( QString, ProdDir );
        ADD_SETTING_VALUE( QStringList, SelectedQtDirs );
        ADD_SETTING_VALUE( QSet< QString >, BuildDirs );
        ADD_SETTING_VALUE( QStringList, InclDirs );
        ADD_SETTING_VALUE( QStringList, SelectedInclDirs );
        ADD_SETTING_VALUE( TExecNameType, ExecNames );
        ADD_SETTING_VALUE( TListOfStringPair, CustomBuilds );
        ADD_SETTING_VALUE( TListOfDebugTargets, DebugCommands );
    }

    void CSettings::loadQtSettings()
    {
        if ( !fQtDir.isValid() || fQtDir.toString().isEmpty() )
            return;

        auto qtDir = QDir( fQtDir.toString() );
        if ( !qtDir.exists() )
            return;

        if ( !qtDir.cd( "include" ) )
            return;

        QDirIterator iter( qtDir.absolutePath(), QStringList(), QDir::Filter::AllDirs | QDir::Filter::NoDotAndDotDot | QDir::Filter::Readable, QDirIterator::IteratorFlag::NoIteratorFlags );
        fQtDirs = QVariant();
        QStringList qtDirs;
        while ( iter.hasNext() )
        {
            qtDirs << QFileInfo( iter.next() ).fileName();
        }
        fQtDirs = qtDirs;
    }

    bool CSettings::loadData()
    {
        auto keys = fSettingsFile->childKeys();
        for ( auto && ii : keys )
        {
            auto pos = fSettings.find( ii );
            if ( pos == fSettings.end() )
                continue;

            auto value = fSettingsFile->value( ii );
            *( *pos ).second = value;
        }
        loadQtSettings();
        return true;
    }

    int CSettings::runCMake( const std::function< void( const QString & ) > & outFunc, const std::function< void( const QString & ) > & errFunc, QProcess * process, const std::pair< bool, std::function< void() > > & finishedInfo ) const
    {
        if ( !process )
            return -1;

        auto buildDir = getBuildDir().value();
        auto cmakeExec = getCMakePath();
        auto args = getCmakeArgs();
        outFunc( QString( "Build Dir: %1" ).arg( buildDir ) + "\n" );
        outFunc( QString( "CMake Path: %1" ).arg( cmakeExec ) + "\n" );
        outFunc( QString( "Args: %1" ).arg( args.join( " " ) ) + "\n" );

        outFunc( QObject::tr( "============================================" ) + "\n" );
        outFunc( QObject::tr( "Running CMake" ) + "\n" );

        QObject::connect( process, &QProcess::readyReadStandardOutput,
                          [process, outFunc]()
        {
            outFunc( process->readAllStandardOutput() );
        } );

        QObject::connect( process, &QProcess::readyReadStandardError,
                          [process, errFunc]()
        {
            errFunc( process->readAllStandardError() );
        } );

        int returnExitCode = -1;
        auto returnExitStatus = QProcess::ExitStatus::NormalExit;

        auto localCallback = new std::function< void() >( finishedInfo.second );
        QObject::connect( process, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ),
                          [process, outFunc, errFunc, &returnExitCode, &returnExitStatus, localCallback ]( int exitCode, QProcess::ExitStatus status )
        {
            QString msg = 
                QString( "============================================\n" )
                + QString( "Finished: Exit Code: %1 Status: %2\n" ).arg( exitCode ).arg( status == QProcess::NormalExit ? "Normal" : "Crashed" );

            returnExitStatus = status;
            returnExitCode = exitCode;
            if ( ( exitCode != 0 ) || ( status != QProcess::ExitStatus::NormalExit ) )
                errFunc( msg );
            else
                outFunc( msg );
            if ( *localCallback )
                ( *localCallback )( );
        } );
        process->setWorkingDirectory( buildDir );
        process->start( cmakeExec, args );

        if ( finishedInfo.first )
        {
            process->waitForFinished( -1 );
            return ( returnExitStatus == QProcess::ExitStatus::NormalExit ) ? returnExitCode : -1;
        }
        else
            return 0;
    }

    QString SSourceFileResults::getText( bool forText ) const
    {
        QString retVal = ( forText ?
                           QObject::tr( "Files: %1 Directories: %2 Executables: %3 Include Directories: %4 Build Directories: %5" ) :
                           QObject::tr(
                           "<ul>"
                           "<li>Files: %1</li>"
                           "<li>Directories: %2</li>"
                           "<li>Executables: %3</li>"
                           "<li>Include Directories: %4</li>"
                           "<li>Build Directories: %5</li>"
                           "</ul>"
                           )
                           ).arg( fFiles )
            .arg( fDirs )
            .arg( fExecutables.count() )
            .arg( fInclDirs.count() )
            .arg( fBuildDirs.count() )
            ;
        return retVal;
    }

    void SSourceFileInfo::createItem( QStandardItem * parent ) const
    {
        auto node = new QStandardItem( fRelToDir );
        QList<QStandardItem *> row;
        node->setData( fIsDir, NVSProjectMaker::ERoles::eIsDirRole );
        node->setData( fRelToDir, NVSProjectMaker::ERoles::eRelPathRole );
        row << node;

        if ( fIsDir )
        {
            node->setData( fIsBuildDir, NVSProjectMaker::ERoles::eIsBuildDirRole );
            node->setData( fIsIncludeDir, NVSProjectMaker::ERoles::eIsIncludeDirRole );
            node->setData( QVariant::fromValue( fExecutables ), NVSProjectMaker::ERoles::eExecutablesRole );

            QStringList execNames;
            for ( auto && ii : fExecutables )
                execNames << ii.first;
            //QStringList guiExecs;
            //for ( auto && ii : fExecutables )
            //    guiExecs << ( ii.second ? "Yes" : "No" );

            auto buildNode = new QStandardItem( fIsBuildDir ? "Yes" : "" );
            auto execNode = new QStandardItem( execNames.join( " " ) );
            auto inclDir = new QStandardItem( fIsIncludeDir ? "Yes" : "" );
            row << buildNode << execNode << inclDir;

            for( auto ii : fChildren )
                ii->createItem( node );
        }

        parent->appendRow( row );
    }

    bool SSourceFileInfo::isPairedInclSrcDir( const QString & srcDir ) const // return true when name is incl or src, and the peer directory exists
    {
        if ( !fRelToDir.endsWith( "incl" ) && !fRelToDir.endsWith( "src" ) )
            return false;

        QDir dir( srcDir );
        dir.cd( fRelToDir );
        dir.cdUp();

        bool otherExists = false;
        if ( fRelToDir.endsWith( "incl" ) )
            otherExists = QFileInfo( dir.absoluteFilePath( "src" ) ).exists();
        else if ( fRelToDir.endsWith( "src" ) )
            otherExists = QFileInfo( dir.absoluteFilePath( "incl" ) ).exists();
        return otherExists;
    }

    bool SSourceFileInfo::isParentToPairedDirs( const QString & srcDir ) const // return true when name is incl or src, and the peer directory exists
    {
        QDir dir( srcDir );
        dir.cd( fRelToDir );
        return QFileInfo( dir.absoluteFilePath( "src" ) ).exists() && QFileInfo( dir.absoluteFilePath( "incl" ) ).exists();
    }

}