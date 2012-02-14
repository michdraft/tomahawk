/*
    Copyright (C) 2011  Leo Franchi <leo.franchi@kdab.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "AccountDelegate.h"

#include <QApplication>
#include <QPainter>
#include <QMouseEvent>

#include "accounts/AccountModel.h"
#include "accounts/Account.h"

#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#define CHILD_ACCOUNT_HEIGHT 24

#define PADDING 4
#define PADDING_BETWEEN_STARS 2
#define STAR_SIZE 12

#ifdef Q_WS_MAC
#define TOPLEVEL_ACCOUNT_HEIGHT 72
#else
#define TOPLEVEL_ACCOUNT_HEIGHT 68
#endif

#define ICONSIZE 40
#define WRENCH_SIZE 24
#define SMALL_WRENCH_SIZE 16
#define STATUS_ICON_SIZE 13
#define CHECK_LEFT_EDGE 8
#define REMOVE_ICON_SIZE 12

using namespace Tomahawk;
using namespace Accounts;

AccountDelegate::AccountDelegate( QObject* parent )
    : QStyledItemDelegate ( parent )
{

    m_defaultCover.load( RESPATH "images/sipplugin-online.png" );
    m_ratingStarPositive.load( RESPATH "images/starred.png" );
    m_ratingStarNegative.load( RESPATH "images/star-unstarred.png" );
    m_onHoverStar.load( RESPATH "images/star-hover.png" );
    m_onlineIcon.load( RESPATH "images/sipplugin-online.png" );
    m_offlineIcon.load( RESPATH "images/sipplugin-offline.png" );
    m_removeIcon.load( RESPATH "images/list-remove.png" );

    m_ratingStarPositive = m_ratingStarPositive.scaled( STAR_SIZE, STAR_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation  );
    m_ratingStarNegative = m_ratingStarNegative.scaled( STAR_SIZE, STAR_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation  );
    m_onlineIcon = m_onlineIcon.scaled( STATUS_ICON_SIZE, STATUS_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation  );
    m_offlineIcon = m_offlineIcon.scaled( STATUS_ICON_SIZE, STATUS_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation  );
    m_onHoverStar = m_onHoverStar.scaled( STAR_SIZE, STAR_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation  );
    m_removeIcon = m_removeIcon.scaled( REMOVE_ICON_SIZE, REMOVE_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation );

    m_defaultCover = m_defaultCover.scaled( ICONSIZE, ICONSIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation );

}


QSize
AccountDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    AccountModel::RowType rowType = static_cast< AccountModel::RowType >( index.data( AccountModel::RowTypeRole ).toInt() );
    if ( rowType == AccountModel::TopLevelAccount || rowType == AccountModel::UniqueFactory )
        return QSize( 200, TOPLEVEL_ACCOUNT_HEIGHT );
    else if ( rowType == AccountModel::TopLevelFactory )
    {
        // Make more space for each account we have to show.
        AccountFactory* fac = qobject_cast< AccountFactory* >( index.data( AccountModel::AccountData ).value< QObject* >() );
        if ( fac->isUnique() )
            return QSize( 200, TOPLEVEL_ACCOUNT_HEIGHT );

        const QList< Account* > accts = index.data( AccountModel::ChildrenOfFactoryRole ).value< QList< Tomahawk::Accounts::Account* > >();
        const QSize s = QSize( 200, TOPLEVEL_ACCOUNT_HEIGHT + 12 * accts.size()-1 );

        if ( s != m_sizeHints[ index ] )
            const_cast< AccountDelegate* >( this )->sizeHintChanged( index ); // FU KTHBBQ

        m_sizeHints[ index ] = s;
        return s;
    }

    return QSize();
}


void
AccountDelegate::paint ( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );

    // draw the background
    const QWidget* w = opt.widget;
    QStyle* style = w ? w->style() : QApplication::style();
    style->drawPrimitive( QStyle::PE_PanelItemViewItem, &opt, painter, w );

    painter->setRenderHint( QPainter::Antialiasing );

    QFont titleFont = opt.font;
    titleFont.setBold( true );
    titleFont.setPointSize( titleFont.pointSize() + 2 );
    const QFontMetrics titleMetrics( titleFont );

    QFont authorFont = opt.font;
    authorFont.setItalic( true );
    authorFont.setPointSize( authorFont.pointSize() - 1 );
    #ifdef Q_OS_MAC
    authorFont.setPointSize( authorFont.pointSize() - 1 );
    #endif
    const QFontMetrics authorMetrics( authorFont );

    QFont descFont = authorFont;
    descFont.setItalic( false );
    const QFontMetrics descMetrics( descFont );

    QFont installFont = opt.font;
    installFont.setPointSize( installFont.pointSize() - 1 );
    const QFontMetrics installMetrics( descFont );

    const int height = opt.rect.height();
    const int center = height / 2 + opt.rect.top();

    // Left account enable/disable checkbox
    const AccountModel::RowType rowType = static_cast< AccountModel::RowType >( index.data( AccountModel::RowTypeRole ).toInt() );
    int leftEdge = PADDING;
    // draw checkbox first
    const int checkboxYPos = ( center ) - ( WRENCH_SIZE / 2 );
    QRect checkRect = QRect( leftEdge, checkboxYPos, WRENCH_SIZE, WRENCH_SIZE );
    QStyleOptionViewItemV4 opt2 = opt;
    opt2.rect = checkRect;
    drawCheckBox( opt2, painter, opt.widget );
    leftEdge += WRENCH_SIZE + PADDING / 2;

    // Pixmap
    QPixmap p = index.data( Qt::DecorationRole ).value< QPixmap >();
    QRect pixmapRect( leftEdge + PADDING, center - ICONSIZE/2, ICONSIZE, ICONSIZE );
    if ( p.isNull() ) // default image... TODO
        p = m_defaultCover;
    else
        p = p.scaled( pixmapRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation );

    painter->drawPixmap( pixmapRect, p );

    // Draw config wrench if there is one
    const bool hasConfigWrench = index.data( AccountModel::HasConfig ).toBool();
    int rightEdge = opt.rect.right();
    m_cachedConfigRects[ index ] = QRect();
    if ( hasConfigWrench )
    {
        const QRect confRect = QRect( rightEdge - 2*PADDING - WRENCH_SIZE, center - WRENCH_SIZE / 2, WRENCH_SIZE, WRENCH_SIZE );
        QStyleOptionToolButton topt;
        topt.rect = confRect;
        topt.pos = confRect.topLeft();

        drawConfigWrench( painter, opt, topt );
        m_cachedConfigRects[ index ] = confRect;
        rightEdge = confRect.left();

    }

    // Draw individual accounts and add account button for factories
    m_cachedButtonRects[ index ] = QRect();

    bool canDelete = index.data( AccountModel::CanDeleteRole ) .toBool();
    if ( rowType == Tomahawk::Accounts::AccountModel::TopLevelFactory )
    {
        const QList< Account* > accts = index.data( AccountModel::ChildrenOfFactoryRole ).value< QList< Tomahawk::Accounts::Account* > >();

        QRect btnRect;
        const QString btnText = tr( "Add Account" );
        const int btnWidth = installMetrics.width( btnText ) + 2*PADDING;

        if ( accts.isEmpty() )
        {
            Q_ASSERT( !hasConfigWrench );

            // Draw button in center of row
            btnRect= QRect( opt.rect.right() - PADDING - btnWidth, center - ( installMetrics.height() + 4 ) / 2, btnWidth, installMetrics.height() + 2*PADDING );
            rightEdge = btnRect.left();
        }
        else
        {
            painter->save();
            painter->setFont( installFont );
            int oldRightEdge = rightEdge;
            rightEdge = drawAccountList( painter, opt, accts, rightEdge );
            painter->restore();

            int centeredUnderAccounts = oldRightEdge - (oldRightEdge - rightEdge)/2 - (btnWidth/2);
            btnRect = QRect( opt.rect.right() - PADDING - btnWidth, opt.rect.bottom() - installMetrics.height() - 3*PADDING,  btnWidth, installMetrics.height() + 2*PADDING );
        }

        leftEdge = btnRect.left();
        m_cachedButtonRects[ index ] = btnRect;

        painter->save();
        painter->setPen( opt.palette.color( QPalette::Active, QPalette::AlternateBase ) );

        drawRoundedButton( painter, btnRect );

        painter->setFont( installFont );
        painter->drawText( btnRect, Qt::AlignCenter, btnText );
        painter->restore();
    }
    else if ( rowType == AccountModel::UniqueFactory )
    {
        // Display as usual, except if it has an account, show the status.
        const QList< Account* > accts = index.data( AccountModel::ChildrenOfFactoryRole ).value< QList< Tomahawk::Accounts::Account* > >();
        if ( !accts.isEmpty() )
        {
            Q_ASSERT( accts.size() == 1 );

            rightEdge = drawStatus( painter, QPointF( rightEdge, center - painter->fontMetrics().height()/2 ), accts.first(), true );
        }

    }
    else if ( canDelete )
    {
        const QString btnText = tr( "Remove Account" );
        const int btnWidth = installMetrics.width( btnText ) + 2*PADDING;
        QRect btnRect;

        if ( hasConfigWrench )
            btnRect = QRect( opt.rect.right() - PADDING - btnWidth, opt.rect.bottom() - installMetrics.height() - 3*PADDING,  btnWidth, installMetrics.height() + 2*PADDING );
        else
            btnRect = QRect( opt.rect.right() - PADDING - btnWidth, center - ( installMetrics.height() + 4 ) / 2, btnWidth, installMetrics.height() + 2*PADDING );

        leftEdge = btnRect.left();
        m_cachedButtonRects[ index ] = btnRect;

        painter->save();
        painter->setPen( opt.palette.color( QPalette::Active, QPalette::AlternateBase ) );

        drawRoundedButton( painter, btnRect, true );

        painter->setFont( installFont );
        painter->drawText( btnRect, Qt::AlignCenter, btnText );
        painter->restore();
    }

    // Draw the title and description
    // title
    QString title = index.data( Qt::DisplayRole ).toString();
    const int rightTitleEdge = rightEdge - PADDING;
    const int leftTitleEdge = pixmapRect.right() + PADDING;
    painter->setFont( titleFont );
    QRect textRect;
    const bool canRate = index.data( AccountModel::CanRateRole ).toBool();
    if ( canRate )
    {
        textRect = QRect( leftTitleEdge, opt.rect.top() + PADDING, rightTitleEdge - leftTitleEdge, painter->fontMetrics().height() );
    }
    else
    {
        textRect = QRect( leftTitleEdge, opt.rect.top() + PADDING, rightTitleEdge - leftTitleEdge, center - opt.rect.top() - PADDING );
    }
    painter->drawText( textRect, Qt::AlignVCenter | Qt::AlignLeft, title );

    // author
    QString author = index.data( AccountModel::AuthorRole ).toString();
    int runningBottom = textRect.bottom();
    if ( !author.isEmpty() && canRate )
    {
        painter->save();
        painter->setFont( authorFont );
        painter->setPen( QColor( Qt::gray ).darker( 150 ) );
        const int authorWidth = authorMetrics.width( author );
        const QRect authorRect( textRect.left(),  textRect.bottom() + PADDING/2, authorWidth + 6, authorMetrics.height() );
        painter->drawText( authorRect, Qt::AlignLeft | Qt::AlignVCenter, author );
        painter->restore();

        runningBottom = authorRect.bottom();
    }

    // description
    QString desc = index.data( AccountModel::DescriptionRole ).toString();
    const int descWidth = rightEdge - leftTitleEdge - PADDING;
    painter->setFont( descFont );
    const QRect descRect( leftTitleEdge, runningBottom + PADDING, descWidth, painter->fontMetrics().height() );
    painter->drawText( descRect, Qt::AlignLeft | Qt::TextWordWrap | Qt::AlignTop, desc );
    runningBottom = descRect.bottom();

    if ( index.data( AccountModel::CanRateRole ).toBool() )
    {
        // rating stars
        const int rating = index.data( AccountModel::RatingRole ).toInt();
        const int ratingWidth = 5 * ( m_ratingStarPositive.width() + PADDING_BETWEEN_STARS );

//         int runningEdge = opt.rect.right() - 2*PADDING - ratingWidth;
        int runningEdge = textRect.left();
//         int starsTop = opt.rect.bottom() - 3*PADDING - m_ratingStarNegative.height();
        int starsTop = runningBottom + PADDING;
        for ( int i = 1; i < 6; i++ )
        {
            QRect r( runningEdge, starsTop, m_ratingStarPositive.width(), m_ratingStarPositive.height() );
//             QRect r( runningEdge, opt.rect.top() + PADDING, m_ratingStarPositive.width(), m_ratingStarPositive.height() );
            if ( i == 1 )
                m_cachedStarRects[ index ] = r;

            const bool userHasRated = index.data( AccountModel::UserHasRatedRole ).toBool();
            if ( !userHasRated && // Show on-hover animation if the user hasn't rated it yet, and is hovering over it
                 m_hoveringOver > -1 &&
                 m_hoveringItem == index )
            {
                if ( i <= m_hoveringOver ) // positive star
                    painter->drawPixmap( r, m_onHoverStar );
                else
                    painter->drawPixmap( r, m_ratingStarNegative );
            }
            else
            {
                if ( i <= rating ) // positive or rated star
                {
                    if ( userHasRated )
                        painter->drawPixmap( r, m_onHoverStar );
                    else
                        painter->drawPixmap( r, m_ratingStarPositive );
                }
                else
                    painter->drawPixmap( r, m_ratingStarNegative );
            }
            runningEdge += m_ratingStarPositive.width() + PADDING_BETWEEN_STARS;
        }

        // downloaded num times
        QString count = tr( "%1 downloads" ).arg( index.data( AccountModel::DownloadCounterRole ).toInt() );
        painter->setFont( descFont );
        const int countW = painter->fontMetrics().width( count );
        const QRect countRect( runningEdge + 50, starsTop, countW, painter->fontMetrics().height() );
        count = painter->fontMetrics().elidedText( count, Qt::ElideRight, rightEdge - PADDING - countRect.left() );
        painter->drawText( countRect, Qt::AlignLeft | Qt::TextWordWrap, count );
        //         runningEdge = authorRect.x();
    }

    // Title and description!
    return;
}


int
AccountDelegate::drawAccountList( QPainter* painter, QStyleOptionViewItemV4& opt, const QList< Account* > accts, int rightEdge ) const
{
    // list each account name, and show the online, offline icon
    const int textHeight = painter->fontMetrics().height() + 1;
    const int mid = opt.rect.bottom() - opt.rect.height() / 2;
    int runningRightEdge = rightEdge;
    int current = 0;

    int leftOfAccounts = 0;

    if ( accts.size() % 2 == 1 )
    {
        // If there's an odd number, the center one is centered
        current = mid - ((textHeight + PADDING/2) * (accts.size()/2) ) - textHeight / 2;
    }
    else
    {
        // Even number, center between the middle ones
        current = mid - ((textHeight + PADDING/2) * (accts.size()/2) );
    }

    for ( int i = 0; i < accts.size(); i++ )
    {
        // draw lightbulb and text
        runningRightEdge = drawStatus( painter, QPointF( rightEdge - PADDING, current), accts.at( i ) );

        const QString label = accts.at( i )->accountFriendlyName();
        const QPoint textTopLeft( runningRightEdge - PADDING - painter->fontMetrics().width( label ), current);
        painter->drawText( QRect( textTopLeft, QSize( painter->fontMetrics().width( label ) + 1, textHeight ) ), label );

        current += textHeight + PADDING/2;

        leftOfAccounts = qMax( leftOfAccounts, textTopLeft.x() );
    }

    return leftOfAccounts;
}


bool
AccountDelegate::editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{
    if ( event->type() != QEvent::MouseButtonPress &&
         event->type() != QEvent::MouseButtonRelease &&
         event->type() != QEvent::MouseButtonDblClick &&
         event->type() != QEvent::MouseMove )
        return false;

    if ( event->type() == QEvent::MouseButtonPress )
    {
        // Show the config wrench as depressed on click
        QMouseEvent* me = static_cast< QMouseEvent* >( event );
        if ( me->button() == Qt::LeftButton && m_cachedConfigRects.contains( index ) && m_cachedConfigRects[ index ].contains( me->pos() ) )
        {
            m_configPressed = index;

            const AccountModel::RowType rowType = static_cast< AccountModel::RowType >( index.data( AccountModel::RowTypeRole ).toInt() );
            if ( rowType == AccountModel::TopLevelAccount )
            {
                Account* acct = qobject_cast< Account* >( index.data( AccountModel::AccountData ).value< QObject* >() );
                Q_ASSERT( acct ); // Should not be showing a config wrench if there is no account!

                emit openConfig( acct );
            }
            else if ( rowType == AccountModel::TopLevelFactory )
            {
                AccountFactory* fac = qobject_cast< AccountFactory* >( index.data( AccountModel::AccountData ).value< QObject* >() );
                Q_ASSERT( fac ); // Should not be showing a config wrench if there is no account!
                emit openConfig( fac );
            }
            else if ( rowType == AccountModel::UniqueFactory )
            {
                const QList< Account* > accts = index.data( AccountModel::ChildrenOfFactoryRole ).value< QList< Tomahawk::Accounts::Account* > >();

                Q_ASSERT( !accts.isEmpty() ); // If there's no account, why is there a config widget for this factory?
                Q_ASSERT( accts.size() == 1 );
                emit openConfig( accts.first() );
            }
            return true;
        }
    } else if ( event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseButtonDblClick )
    {
        QMouseEvent* me = static_cast< QMouseEvent* >( event );
        if ( m_configPressed.isValid() )
            emit update( m_configPressed );

        m_configPressed = QModelIndex();

        const AccountModel::ItemState state = static_cast< AccountModel::ItemState >( index.data( AccountModel::StateRole ).toInt() );

        if ( checkRectForIndex( option, index ).contains( me->pos() ) )
        {
            // Check box for this row

            // eat the double click events inside the check rect
            if( event->type() == QEvent::MouseButtonDblClick ) {
                return true;
            }

            Qt::CheckState curState = static_cast< Qt::CheckState >( index.data( Qt::CheckStateRole ).toInt() );
            Qt::CheckState newState = curState == Qt::Checked ? Qt::Unchecked : Qt::Checked;
            return model->setData( index, newState, AccountModel::CheckboxClickedRole );
        }
        else if ( m_cachedButtonRects.contains( index ) && m_cachedButtonRects[ index ].contains( me->pos() ) )
        {
            // Install/create/etc button for this row
            model->setData( index, true, AccountModel::CustomButtonRole );
        }
    }

    if ( m_cachedStarRects.contains( index ) )
    {
        QRect fullStars = m_cachedStarRects[ index ];
        const int starsWidth = 5 * ( m_ratingStarPositive.width() + PADDING_BETWEEN_STARS );
        fullStars.setWidth( starsWidth );

        QMouseEvent* me = static_cast< QMouseEvent* >( event );

        if ( fullStars.contains( me->pos() ) )
        {
            const int eachStar = starsWidth / 5;
            const int clickOffset = me->pos().x() - fullStars.x();
            const int whichStar = (clickOffset / eachStar) + 1;

            if ( event->type() == QEvent::MouseButtonRelease )
            {
                model->setData( index, whichStar, AccountModel::RatingRole );
            }
            else if ( event->type() == QEvent::MouseMove )
            {
                // 0-indexed
                m_hoveringOver = whichStar;
                m_hoveringItem = index;
            }

            return true;
        }
    }

    if ( m_hoveringOver > -1 )
    {
        emit update( m_hoveringItem );
        m_hoveringOver = -1;
        m_hoveringItem = QPersistentModelIndex();
    }
    return false;
}


void
AccountDelegate::drawRoundedButton( QPainter* painter, const QRect& btnRect, bool red ) const
{
    QPainterPath btnPath;
    const int radius = 3;
    // draw top half gradient
    const int btnCenter = btnRect.bottom() - ( btnRect.height() / 2 );
    btnPath.moveTo( btnRect.left(), btnCenter );
    btnPath.lineTo( btnRect.left(), btnRect.top() + radius );
    btnPath.quadTo( QPoint( btnRect.topLeft() ), QPoint( btnRect.left() + radius, btnRect.top() ) );
    btnPath.lineTo( btnRect.right() - radius, btnRect.top() );
    btnPath.quadTo( QPoint( btnRect.topRight() ), QPoint( btnRect.right(), btnRect.top() + radius ) );
    btnPath.lineTo( btnRect.right(),btnCenter );
    btnPath.lineTo( btnRect.left(), btnCenter );

    QLinearGradient g;
    if ( !red )
    {
        g.setColorAt( 0, QColor(54, 127, 211) );
        g.setColorAt( 0.5, QColor(43, 104, 182) );
    }
    else
    {
        g.setColorAt( 0, QColor(206, 63, 63) );
        g.setColorAt( 0.5, QColor(170, 52, 52) );
    }
    //painter->setPen( bg.darker() );
    painter->fillPath( btnPath, g );
    //painter->drawPath( btnPath );

    btnPath = QPainterPath();
    btnPath.moveTo( btnRect.left(), btnCenter );
    btnPath.lineTo( btnRect.left(), btnRect.bottom() - radius );
    btnPath.quadTo( QPoint( btnRect.bottomLeft() ), QPoint( btnRect.left() + radius, btnRect.bottom() ) );
    btnPath.lineTo( btnRect.right() - radius, btnRect.bottom() );
    btnPath.quadTo( QPoint( btnRect.bottomRight() ), QPoint( btnRect.right(), btnRect.bottom() - radius ) );
    btnPath.lineTo( btnRect.right(), btnCenter );
    btnPath.lineTo( btnRect.left(), btnCenter );

    if ( !red )
    {
        g.setColorAt( 0, QColor(34, 85, 159) );
        g.setColorAt( 0.5, QColor(35, 79, 147) );
    }
    else
    {
        g.setColorAt( 0, QColor(150, 50, 50) );
        g.setColorAt( 0.5, QColor(130, 40, 40) );
    }
    painter->fillPath( btnPath, g );
}


int
AccountDelegate::drawStatus( QPainter* painter, const QPointF& rightTopEdge, Account* acct, bool drawText ) const
{
    QPixmap p;
    QString statusText;
    Account::ConnectionState state = acct->connectionState();
    if ( state == Account::Connected )
    {
        p = m_onlineIcon;
        statusText = tr( "Online" );
    }
    else if ( state == Account::Connecting )
    {
        p = m_offlineIcon;
        statusText = tr( "Connecting..." );
    }
    else
    {
        p = m_offlineIcon;
        statusText = tr( "Offline" );
    }

    const int yPos = rightTopEdge.y();
    const QRect connectIconRect( rightTopEdge.x() - STATUS_ICON_SIZE, yPos, STATUS_ICON_SIZE, STATUS_ICON_SIZE );
    painter->drawPixmap( connectIconRect, p );

    int leftEdge = connectIconRect.x();
    if ( drawText )
    {
        int width = painter->fontMetrics().width( statusText );
        int statusTextX = connectIconRect.x() - PADDING - width;
        painter->drawText( QRect( statusTextX, yPos, width, painter->fontMetrics().height() ), statusText );

        leftEdge = statusTextX;
    }

    return leftEdge;
}


void
AccountDelegate::drawCheckBox( QStyleOptionViewItemV4& opt, QPainter* p, const QWidget* w ) const
{
    QStyle* style = w ? w->style() : QApplication::style();
    opt.checkState == Qt::Checked ? opt.state |= QStyle::State_On : opt.state |= QStyle::State_Off;
    style->drawPrimitive( QStyle::PE_IndicatorViewItemCheck, &opt, p, w );
}


void
AccountDelegate::drawConfigWrench ( QPainter* painter, QStyleOptionViewItemV4& opt, QStyleOptionToolButton& topt ) const
{
    const QWidget* w = opt.widget;
    QStyle* style = w ? w->style() : QApplication::style();

    // draw it the same size as the check belox
    topt.font = opt.font;
    topt.icon = QIcon( RESPATH "images/configure.png" );
    topt.iconSize = QSize( 14, 14 );
    topt.subControls = QStyle::SC_ToolButton;
    topt.activeSubControls = QStyle::SC_None;
    topt.features = QStyleOptionToolButton::None;
    bool pressed = ( m_configPressed == opt.index );
    topt.state = pressed ? QStyle::State_On : QStyle::State_Raised;
    if( opt.state & QStyle::State_MouseOver || pressed )
        topt.state |= QStyle::State_HasFocus;
    style->drawComplexControl( QStyle::CC_ToolButton, &topt, painter, w );
}



QRect
AccountDelegate::checkRectForIndex( const QStyleOptionViewItem& option, const QModelIndex& idx ) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, idx );

    // Top level item, return the corresponding rect
    const int ypos = ( opt.rect.top() + opt.rect.height() / 2 ) - ( WRENCH_SIZE / 2 );
    const QRect checkRect = QRect( PADDING, ypos, WRENCH_SIZE, WRENCH_SIZE );

    return checkRect;

}


