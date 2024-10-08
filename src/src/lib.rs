use anchor_lang::prelude::*;

declare_id!("4yTRVbWqfvr8TGLpFhGof33aJ9LKaFMWDo5upjJnMB4m");

#[program]
pub mod race_bet {
    use super::*;

    pub fn place_bet(ctx: Context<PlaceBet>, amount: u64) -> Result<()> {
        let bet = &mut ctx.accounts.bet;
        bet.player1 = *ctx.accounts.player1.key;
        bet.player2 = *ctx.accounts.player2.key;
        bet.amount = amount;

                                 
        let ix1 = anchor_lang::solana_program::system_instruction::transfer(                         
            &ctx.accounts.player1.key(),                                                             
            &ctx.accounts.escrow.key(),                                                              
            amount,                                                                                  
        );                                                                                           
        anchor_lang::solana_program::program::invoke(                                                
            &ix1,                                                                                    
            &[                                                                                       
                ctx.accounts.player1.to_account_info(),                                              
                ctx.accounts.escrow.to_account_info(),                                               
            ],                                                                                       
        )?;                                                                                          
                                                                                                     
        let ix2 = anchor_lang::solana_program::system_instruction::transfer(                         
            &ctx.accounts.player2.key(),                                                             
            &ctx.accounts.escrow.key(),                                                              
            amount,                                                                                  
        );                                                                                           
        anchor_lang::solana_program::program::invoke(                                                
            &ix2,                                                                                    
            &[                                                                                       
                ctx.accounts.player2.to_account_info(),                                              
                ctx.accounts.escrow.to_account_info(),                                               
            ],                                                                                       
        )?;      
        Ok(())
    }

    pub fn settle_bet(ctx: Context<SettleBet>) -> Result<()> {
        let bet = &ctx.accounts.bet;

        // Connection to game's API in progress
        // Just for now, money will go to the first player
        let winner = &ctx.accounts.player1;
        let commission_account = &ctx.accounts.commission;

        let total_amount = bet.amount * 2;
        let winner_amount = total_amount * 90 / 100;
        let commission_amount = total_amount * 10 / 100;
                                                        
        let ix1 = anchor_lang::solana_program::system_instruction::transfer(                         
            &ctx.accounts.escrow.key(),                                                              
            &winner.key(),                                                                           
            winner_amount,                                                                           
        );                                                                                           
        anchor_lang::solana_program::program::invoke(                                                
            &ix1,                                                                                    
            &[                                                                                       
                ctx.accounts.escrow.to_account_info(),                                               
                winner.to_account_info(),                                                            
            ],                                                                                       
        )?;                                                                                          
                                                                                                     
        let ix2 = anchor_lang::solana_program::system_instruction::transfer(                         
            &ctx.accounts.escrow.key(),                                                              
            &commission_account.key(),                                                               
            commission_amount,                                                                       
        );                                                                                           
        anchor_lang::solana_program::program::invoke(                                                
            &ix2,                                                                                    
            &[                                                                                       
                ctx.accounts.escrow.to_account_info(),                                               
                commission_account.to_account_info(),                                                
            ],                                                                                       
        )?;       

        Ok(())
    }
}

#[derive(Accounts)]
pub struct PlaceBet<'info> {
    #[account(mut)]
    pub player1: Signer<'info>,
    #[account(mut)]
    pub player2: Signer<'info>,
    /// CHECK: This is an unchecked account, used for holding the escrow funds.
    #[account(mut)]
    pub escrow: AccountInfo<'info>,
    #[account(init, payer = player1, space = 8 + 32 + 32 + 8)]
    pub bet: Account<'info, Bet>,
    pub system_program: Program<'info, System>,
}

#[derive(Accounts)]
pub struct SettleBet<'info> {
    #[account(mut)]
    pub player1: Signer<'info>,
    #[account(mut)]
    pub player2: Signer<'info>,
    /// CHECK: This is an unchecked account, used for holding the escrow funds.     
    #[account(mut)]
    pub escrow: AccountInfo<'info>,
    /// CHECK: This is an unchecked account, used for holding the commission funds.  
    #[account(mut)]
    pub commission: AccountInfo<'info>,
    #[account(mut, close = player1)]
    pub bet: Account<'info, Bet>,
    pub system_program: Program<'info, System>,
}

#[account]
pub struct Bet {
    pub player1: Pubkey,
    pub player2: Pubkey,
    pub amount: u64,
}
